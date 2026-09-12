#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR } LogLevel;

static inline void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...) {
    static const char *level_strs[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    
    // Get timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", t);

    FILE *out = (level == LOG_ERROR) ? stderr : stdout;
    fprintf(out, "[%s] [%s] (%s:%d): ", time_buf, level_strs[level], file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
    fprintf(out, "\n");
}

#define LOG_DEBUG(fmt, ...) log_msg(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_msg(LOG_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_msg(LOG_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_msg(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif
