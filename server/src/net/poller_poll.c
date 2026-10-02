/* Portable poll() backend. O(n) per wait, fine for thousands of connections; Linux uses epoll instead. */
#include <errno.h>
#include <stdlib.h>

#include "net/poller.h"

#ifdef _WIN32
typedef WSAPOLLFD pfd_t;
#define DO_POLL(f, n, t) WSAPoll((f), (ULONG)(n), (t))
#else
#include <poll.h>
typedef struct pollfd pfd_t;
#define DO_POLL(f, n, t) poll((f), (nfds_t)(n), (t))
#endif

struct poller {
    pfd_t *fds;
    void **ptrs;
    size_t n, cap;
    size_t cursor; /* rotating start index so busy low fds cannot starve high ones */
};

poller_t *poller_create(void) { return calloc(1, sizeof(poller_t)); }

void poller_destroy(poller_t *p) {
    if (!p) return;
    free(p->fds);
    free(p->ptrs);
    free(p);
}

static short events_for(int want_write) { return (short)(POLLIN | (want_write ? POLLOUT : 0)); }

int poller_add(poller_t *p, sock_t fd, void *ptr, int want_write) {
    if (p->n == p->cap) {
        size_t cap = p->cap ? p->cap * 2 : 64;
        pfd_t *f = realloc(p->fds, cap * sizeof *f);
        if (!f) return -1;
        p->fds = f;
        void **q = realloc(p->ptrs, cap * sizeof *q);
        if (!q) return -1;
        p->ptrs = q;
        p->cap = cap;
    }
    p->fds[p->n].fd = fd;
    p->fds[p->n].events = events_for(want_write);
    p->fds[p->n].revents = 0;
    p->ptrs[p->n] = ptr;
    p->n++;
    return 0;
}

static long find(poller_t *p, sock_t fd) {
    for (size_t i = 0; i < p->n; i++)
        if (p->fds[i].fd == fd) return (long)i;
    return -1;
}

int poller_set_write(poller_t *p, sock_t fd, void *ptr, int want_write) {
    long i = find(p, fd);
    if (i < 0) return -1;
    p->fds[i].events = events_for(want_write);
    p->ptrs[i] = ptr;
    return 0;
}

void poller_remove(poller_t *p, sock_t fd) {
    long i = find(p, fd);
    if (i < 0) return;
    p->n--;
    p->fds[i] = p->fds[p->n];
    p->ptrs[i] = p->ptrs[p->n];
}

int poller_wait(poller_t *p, poll_event_t *out, int max, int timeout_ms) {
    if (p->n == 0) return 0;
    int rc = DO_POLL(p->fds, p->n, timeout_ms);
    if (rc < 0) {
#ifdef _WIN32
        return -1;
#else
        return errno == EINTR ? 0 : -1;
#endif
    }
    int cnt = 0;
    if (p->cursor >= p->n) p->cursor = 0;
    for (size_t k = 0; k < p->n && cnt < max; k++) {
        size_t i = (p->cursor + k) % p->n;
        short re = p->fds[i].revents;
        if (!re) continue;
        out[cnt].ptr = p->ptrs[i];
        out[cnt].readable = (re & (POLLIN | POLLHUP)) != 0;
        out[cnt].writable = (re & POLLOUT) != 0;
        out[cnt].error = (re & (POLLERR | POLLNVAL)) != 0;
        cnt++;
        p->cursor = i + 1;
    }
    return cnt;
}
