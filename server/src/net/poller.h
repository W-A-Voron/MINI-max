/* Readiness notification: epoll on Linux, poll()/WSAPoll() elsewhere (Windows, macOS, BSD). */
#ifndef MINIMAX_POLLER_H
#define MINIMAX_POLLER_H

#include "net/sock.h"

typedef struct poller poller_t;

typedef struct {
    void *ptr;     /* the pointer given to poller_add */
    int readable;  /* data to read, or the peer hung up (recv will return 0) */
    int writable;
    int error;
} poll_event_t;

poller_t *poller_create(void);
void poller_destroy(poller_t *p);
int poller_add(poller_t *p, sock_t fd, void *ptr, int want_write);
int poller_set_write(poller_t *p, sock_t fd, void *ptr, int want_write);
void poller_remove(poller_t *p, sock_t fd);
/* Returns number of events (0 on timeout/interrupt), -1 on error. */
int poller_wait(poller_t *p, poll_event_t *out, int max, int timeout_ms);

#endif
