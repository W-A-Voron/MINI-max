#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "net/poller.h"

struct poller {
    int ep;
};

poller_t *poller_create(void) {
    poller_t *p = calloc(1, sizeof *p);
    if (!p) return NULL;
    p->ep = epoll_create1(EPOLL_CLOEXEC);
    if (p->ep < 0) { free(p); return NULL; }
    return p;
}

void poller_destroy(poller_t *p) {
    if (!p) return;
    close(p->ep);
    free(p);
}

static int ctl(poller_t *p, int op, sock_t fd, void *ptr, int want_write) {
    struct epoll_event ev;
    ev.events = EPOLLIN | (want_write ? EPOLLOUT : 0);
    ev.data.ptr = ptr;
    return epoll_ctl(p->ep, op, fd, &ev);
}

int poller_add(poller_t *p, sock_t fd, void *ptr, int want_write) { return ctl(p, EPOLL_CTL_ADD, fd, ptr, want_write); }
int poller_set_write(poller_t *p, sock_t fd, void *ptr, int want_write) { return ctl(p, EPOLL_CTL_MOD, fd, ptr, want_write); }
void poller_remove(poller_t *p, sock_t fd) { epoll_ctl(p->ep, EPOLL_CTL_DEL, fd, NULL); }

int poller_wait(poller_t *p, poll_event_t *out, int max, int timeout_ms) {
    struct epoll_event evs[256];
    if (max > 256) max = 256;
    int n = epoll_wait(p->ep, evs, max, timeout_ms);
    if (n < 0) return errno == EINTR ? 0 : -1;
    for (int i = 0; i < n; i++) {
        out[i].ptr = evs[i].data.ptr;
        out[i].readable = (evs[i].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP)) != 0;
        out[i].writable = (evs[i].events & EPOLLOUT) != 0;
        out[i].error = (evs[i].events & EPOLLERR) != 0;
    }
    return n;
}
