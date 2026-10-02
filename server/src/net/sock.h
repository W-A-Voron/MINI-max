/* Thin portability layer over BSD sockets (Linux/macOS/BSD) and Winsock (Windows). */
#ifndef MINIMAX_SOCK_H
#define MINIMAX_SOCK_H

#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define SOCK_INVALID INVALID_SOCKET
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif
#else
#include <netdb.h>
#include <sys/socket.h>
typedef int sock_t;
#define SOCK_INVALID (-1)
#endif

int sock_startup(void); /* WSAStartup on Windows, no-op elsewhere. 0 = ok */
void sock_shutdown(void);
void sock_close(sock_t s);
int sock_set_nonblocking(sock_t s);
int sock_set_listen_reuse(sock_t s); /* SO_REUSEADDR (POSIX) / SO_EXCLUSIVEADDRUSE (Windows) */

/* >0 bytes transferred; 0 = peer closed (recv only); <0 = error, inspect sock_would_block()/sock_interrupted() */
long sock_recv(sock_t s, void *buf, size_t n);
long sock_send(sock_t s, const void *buf, size_t n);

int sock_would_block(void); /* last socket error was "try again later" */
int sock_interrupted(void); /* last socket error was EINTR */
const char *sock_errstr(void); /* text of the last socket error */

#endif
