#include "util/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static SRWLOCK g_mu = SRWLOCK_INIT;
#define LOCK() AcquireSRWLockExclusive(&g_mu)
#define UNLOCK() ReleaseSRWLockExclusive(&g_mu)
#else
#include <pthread.h>
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&g_mu)
#define UNLOCK() pthread_mutex_unlock(&g_mu)
#endif

static log_level_t g_level = LOG_INFO;

int log_set_level(const char *name) {
    static const char *names[] = {"debug", "info", "warn", "error"};
    for (int i = 0; i < 4; i++)
        if (strcmp(name, names[i]) == 0) { g_level = (log_level_t)i; return 0; }
    return -1;
}

void log_write(log_level_t lvl, const char *fmt, ...) {
    if (lvl < g_level) return;
    static const char *tag[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};
    char tbuf[32];
    int ms;
#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    snprintf(tbuf, sizeof tbuf, "%04d-%02d-%02dT%02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour,
             st.wMinute, st.wSecond);
    ms = st.wMilliseconds;
#else
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    time_t sec = ts.tv_sec;
    gmtime_r(&sec, &tm);
    strftime(tbuf, sizeof tbuf, "%Y-%m-%dT%H:%M:%S", &tm);
    ms = (int)(ts.tv_nsec / 1000000);
#endif
    LOCK();
    fprintf(stderr, "%s.%03dZ %s ", tbuf, ms, tag[lvl]);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
    UNLOCK();
}
