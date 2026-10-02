#include "net/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minimax/mmproto.h"
#include "net/dispatch.h"
#include "net/poller.h"
#include "net/sock.h"
#include "util/log.h"

typedef struct conn {
    sock_t fd;
    int want_out;
    mm_buf in, out;
    char peer[80];
    struct conn *prev, *next;
} conn_t;

typedef struct {
    const server_config *cfg;
    poller_t *poll;
    sock_t lfd;
    conn_t *head;
    int nconn;
} server_t;

static sock_t listen_socket(const server_config *cfg) {
    struct addrinfo hints, *res, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    char port[16];
    snprintf(port, sizeof port, "%d", cfg->port);
    int rc = getaddrinfo(cfg->host, port, &hints, &res);
    if (rc) { LOGE("getaddrinfo(%s) failed (%d)", cfg->host, rc); return SOCK_INVALID; }
    sock_t fd = SOCK_INVALID;
    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd == SOCK_INVALID) continue;
        sock_set_listen_reuse(fd);
        if (bind(fd, ai->ai_addr, (int)ai->ai_addrlen) == 0 && listen(fd, cfg->backlog) == 0 &&
            sock_set_nonblocking(fd) == 0)
            break;
        LOGE("bind/listen %s:%d: %s", cfg->host, cfg->port, sock_errstr());
        sock_close(fd);
        fd = SOCK_INVALID;
    }
    freeaddrinfo(res);
    return fd;
}

static void conn_close(server_t *s, conn_t *c) {
    LOGI("disconnect %s", c->peer);
    poller_remove(s->poll, c->fd);
    sock_close(c->fd);
    if (c->prev) c->prev->next = c->next; else s->head = c->next;
    if (c->next) c->next->prev = c->prev;
    mm_buf_free(&c->in);
    mm_buf_free(&c->out);
    free(c);
    s->nconn--;
}

static void set_want_out(server_t *s, conn_t *c, int on) {
    if (c->want_out == on) return;
    poller_set_write(s->poll, c->fd, c, on);
    c->want_out = on;
}

/* Returns -1 if the connection was closed. */
static int flush_out(server_t *s, conn_t *c) {
    while (c->out.len) {
        long n = sock_send(c->fd, c->out.data, c->out.len);
        if (n > 0) { mm_buf_consume(&c->out, (size_t)n); continue; }
        if (n < 0 && sock_interrupted()) continue;
        if (n < 0 && sock_would_block()) break;
        conn_close(s, c);
        return -1;
    }
    set_want_out(s, c, c->out.len > 0);
    return 0;
}

static void on_readable(server_t *s, conn_t *c) {
    uint8_t tmp[16384];
    for (;;) {
        long n = sock_recv(c->fd, tmp, sizeof tmp);
        if (n > 0) {
            if (mm_buf_append(&c->in, tmp, (size_t)n)) { conn_close(s, c); return; }
        } else if (n == 0) {
            conn_close(s, c);
            return;
        } else if (sock_interrupted()) {
            continue;
        } else if (sock_would_block()) {
            break;
        } else {
            conn_close(s, c);
            return;
        }
    }
    size_t off = 0;
    for (;;) {
        mm_frame f;
        size_t used = 0;
        mm_status st = mm_frame_decode(c->in.data + off, c->in.len - off, s->cfg->max_frame_bytes, &f, &used);
        if (st == MM_NEED_MORE) break;
        if (st != MM_OK) {
            LOGW("%s: protocol error %d, closing", c->peer, (int)st);
            conn_close(s, c);
            return;
        }
        LOGD("%s: frame type=%u req=%u len=%u", c->peer, f.type, f.request_id, f.payload_len);
        if (dispatch_frame(s->cfg, &f, &c->out)) { conn_close(s, c); return; }
        off += used;
    }
    mm_buf_consume(&c->in, off);
    flush_out(s, c);
}

static void on_accept(server_t *s) {
    for (;;) {
        struct sockaddr_storage ss;
        socklen_t sl = sizeof ss;
        sock_t fd = accept(s->lfd, (struct sockaddr *)&ss, &sl);
        if (fd == SOCK_INVALID) {
            if (sock_interrupted()) continue;
            if (!sock_would_block()) LOGW("accept: %s", sock_errstr());
            return;
        }
        if (sock_set_nonblocking(fd) != 0) { sock_close(fd); continue; }
        if (s->nconn >= s->cfg->max_connections) {
            LOGW("connection limit reached, rejecting");
            sock_close(fd);
            continue;
        }
        conn_t *c = calloc(1, sizeof *c);
        if (!c) { sock_close(fd); continue; }
        c->fd = fd;
        mm_buf_init(&c->in);
        mm_buf_init(&c->out);
        char host[64] = "?", serv[16] = "?";
        getnameinfo((struct sockaddr *)&ss, sl, host, sizeof host, serv, sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV);
        snprintf(c->peer, sizeof c->peer, "%s:%s", host, serv);
        if (poller_add(s->poll, fd, c, 0) < 0) { sock_close(fd); free(c); continue; }
        c->next = s->head;
        if (s->head) s->head->prev = c;
        s->head = c;
        s->nconn++;
        LOGI("connect %s (%d total)", c->peer, s->nconn);
    }
}

int server_run(const server_config *cfg, volatile sig_atomic_t *stop) {
    server_t s;
    memset(&s, 0, sizeof s);
    s.cfg = cfg;
    s.lfd = listen_socket(cfg);
    if (s.lfd == SOCK_INVALID) return -1;
    s.poll = poller_create();
    if (!s.poll || poller_add(s.poll, s.lfd, NULL, 0) < 0) {
        LOGE("cannot create event poller");
        sock_close(s.lfd);
        poller_destroy(s.poll);
        return -1;
    }
    LOGI("listening on %s:%d", cfg->host, cfg->port);

    poll_event_t evs[128];
    while (!*stop) {
        int n = poller_wait(s.poll, evs, 128, 500);
        if (n < 0) { LOGE("poll failed: %s", sock_errstr()); break; }
        for (int i = 0; i < n; i++) {
            conn_t *c = evs[i].ptr;
            if (!c) { on_accept(&s); continue; }
            if (evs[i].error) { conn_close(&s, c); continue; }
            if (evs[i].writable && flush_out(&s, c) < 0) continue;
            if (evs[i].readable) on_readable(&s, c);
        }
    }
    while (s.head) conn_close(&s, s.head);
    poller_destroy(s.poll);
    sock_close(s.lfd);
    LOGI("server stopped");
    return 0;
}
