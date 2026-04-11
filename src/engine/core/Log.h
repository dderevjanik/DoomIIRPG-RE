#ifndef __LOG_H__
#define __LOG_H__

#include <cstdio>
#include <cstdarg>

// Log levels — higher value = more important
enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_NONE  = 4,
};

// Compile-time minimum level (strip debug logs from release builds)
#ifndef LOG_MIN_LEVEL
#ifdef NDEBUG
#define LOG_MIN_LEVEL LOG_LEVEL_INFO
#else
#define LOG_MIN_LEVEL LOG_LEVEL_DEBUG
#endif
#endif

// Runtime log level — defaults to compile-time minimum
inline LogLevel& logLevel() {
    static LogLevel level = static_cast<LogLevel>(LOG_MIN_LEVEL);
    return level;
}

inline void logSetLevel(LogLevel level) { logLevel() = level; }
inline LogLevel logGetLevel() { return logLevel(); }

// Core logging function
inline void logMsg(LogLevel level, const char* fmt, ...) {
    if (level < logLevel()) return;

    static const char* prefixes[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    if (level <= LOG_LEVEL_ERROR) {
        std::fprintf(stderr, "[%s] ", prefixes[level]);
    }

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
}

// Convenience macros — compile-time elimination when below LOG_MIN_LEVEL
#if LOG_MIN_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) logMsg(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) logMsg(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) logMsg(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) ((void)0)
#endif

#define LOG_ERROR(fmt, ...) logMsg(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#endif
