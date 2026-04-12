#pragma once
#include <format>
#include <print>
#include <source_location>
#include <string_view>

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

// Type-safe logging using std::print (C++23).
// Accepts std::format syntax: "{}", "{:02d}", etc.
template <typename... Args>
inline void logPrint(LogLevel level, std::source_location loc,
                     std::format_string<Args...> fmt, Args&&... args) {
    if (level < logLevel()) return;

    static const char* prefixes[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    if (level <= LOG_LEVEL_ERROR) {
        if (level == LOG_LEVEL_INFO) {
            std::print(stderr, "[{}] ", prefixes[level]);
        } else {
            std::print(stderr, "[{} {}:{}] ", prefixes[level], loc.file_name(), loc.line());
        }
    }

    std::print(stderr, fmt, std::forward<Args>(args)...);
}

// Overload without source location (for LOG_INFO)
template <typename... Args>
inline void logPrint(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    if (level < logLevel()) return;

    static const char* prefixes[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    if (level <= LOG_LEVEL_ERROR) {
        std::print(stderr, "[{}] ", prefixes[level]);
    }

    std::print(stderr, fmt, std::forward<Args>(args)...);
}

// Convenience macros — compile-time elimination when below LOG_MIN_LEVEL
// std::source_location::current() is captured at the macro call site
#if LOG_MIN_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) logPrint(LOG_LEVEL_DEBUG, std::source_location::current(), fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) logPrint(LOG_LEVEL_INFO, fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) logPrint(LOG_LEVEL_WARN, std::source_location::current(), fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) ((void)0)
#endif

#define LOG_ERROR(fmt, ...) logPrint(LOG_LEVEL_ERROR, std::source_location::current(), fmt __VA_OPT__(,) __VA_ARGS__)
