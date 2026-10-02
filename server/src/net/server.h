#ifndef MINIMAX_SERVER_H
#define MINIMAX_SERVER_H

#include <signal.h>

#include "config.h"

/* Runs the epoll reactor until *stop becomes non-zero. Returns 0 on clean shutdown. */
int server_run(const server_config *cfg, volatile sig_atomic_t *stop);

#endif
