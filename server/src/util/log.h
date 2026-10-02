#ifndef MINIMAX_LOG_H
#define MINIMAX_LOG_H

typedef enum { LOG_DEBUG = 0, LOG_INFO, LOG_WARN, LOG_ERROR } log_level_t;

#if defined(__GNUC__)
#define MM_PRINTF_FMT(a, b) __attribute__((format(printf, a, b)))
#else
#define MM_PRINTF_FMT(a, b)
#endif

int log_set_level(const char *name); /* returns 0 on success, -1 for unknown name */
void log_write(log_level_t lvl, const char *fmt, ...) MM_PRINTF_FMT(2, 3);

#define LOGD(...) log_write(LOG_DEBUG, __VA_ARGS__)
#define LOGI(...) log_write(LOG_INFO, __VA_ARGS__)
#define LOGW(...) log_write(LOG_WARN, __VA_ARGS__)
#define LOGE(...) log_write(LOG_ERROR, __VA_ARGS__)

#endif
