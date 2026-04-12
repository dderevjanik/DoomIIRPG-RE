#pragma once
#include <cstdio>
#include <cstdarg>
#include <source_location>

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

// Core logging function (with source location for DEBUG/WARN/ERROR)
inline void logMsg(LogLevel level, std::source_location loc, const char* fmt, ...) {
    if (level < logLevel()) return;

    static const char* prefixes[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    if (level <= LOG_LEVEL_ERROR) {
        if (level == LOG_LEVEL_INFO) {
            std::fprintf(stderr, "[%s] ", prefixes[level]);
        } else {
            // Include file:line for DEBUG, WARN, ERROR
            std::fprintf(stderr, "[%s %s:%u] ", prefixes[level], loc.file_name(), loc.line());
        }
    }

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
}

// Overload without source_location for backward compat (used internally)
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
// std::source_location::current() is captured at the macro call site
#if LOG_MIN_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) logMsg(LOG_LEVEL_DEBUG, std::source_location::current(), fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) logMsg(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) logMsg(LOG_LEVEL_WARN, std::source_location::current(), fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) ((void)0)
#endif

#define LOG_ERROR(fmt, ...) logMsg(LOG_LEVEL_ERROR, std::source_location::current(), fmt, ##__VA_ARGS__)
