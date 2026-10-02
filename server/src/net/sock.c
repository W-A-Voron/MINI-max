#include "net/sock.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

int sock_startup(void) {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;
#else
    return 0;
#endif
}

void sock_shutdown(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

void sock_close(sock_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

int sock_set_nonblocking(sock_t s) {
#ifdef _WIN32
    u_long one = 1;
    return ioctlsocket(s, FIONBIO, &one) == 0 ? 0 : -1;
#else
    int fl = fcntl(s, F_GETFL, 0);
    if (fl < 0 || fcntl(s, F_SETFL, fl | O_NONBLOCK) < 0) return -1;
    (void)fcntl(s, F_SETFD, FD_CLOEXEC);
    return 0;
#endif
}

int sock_set_listen_reuse(sock_t s) {
#ifdef _WIN32
    /* SO_REUSEADDR on Windows would allow port hijacking; exclusive use is the safe equivalent. */
    BOOL on = TRUE;
    return setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&on, sizeof on);
#else
    int one = 1;
    return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
#endif
}

long sock_recv(sock_t s, void *buf, size_t n) {
#ifdef _WIN32
    int r = recv(s, (char *)buf, n > 0x7fffffff ? 0x7fffffff : (int)n, 0);
    return r == SOCKET_ERROR ? -1 : r;
#else
    return (long)recv(s, buf, n, 0);
#endif
}

long sock_send(sock_t s, const void *buf, size_t n) {
#ifdef _WIN32
    int r = send(s, (const char *)buf, n > 0x7fffffff ? 0x7fffffff : (int)n, 0);
    return r == SOCKET_ERROR ? -1 : r;
#elif defined(MSG_NOSIGNAL)
    return (long)send(s, buf, n, MSG_NOSIGNAL);
#else
    return (long)send(s, buf, n, 0);
#endif
}

int sock_would_block(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

int sock_interrupted(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

const char *sock_errstr(void) {
#ifdef _WIN32
    static char buf[256];
    DWORD code = (DWORD)WSAGetLastError();
    DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, buf,
                             (DWORD)sizeof buf, NULL);
    while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == ' ')) buf[--n] = 0;
    if (n == 0) snprintf(buf, sizeof buf, "winsock error %lu", (unsigned long)code);
    return buf;
#else
    return strerror(errno);
#endif
}
