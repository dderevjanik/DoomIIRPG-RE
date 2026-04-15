#pragma once
#include <format>
#include <source_location>

// Fatal error handler — each target (engine, converter) provides its own implementation.
// Uses C variadics for compatibility with existing reverse-engineered code.
void Error(const char* fmt, ...);

// Implementation in Error.cpp — type-safe fatal error with automatic source location.
// Use this in new code instead of Error().
[[noreturn]] void fatalErrorImpl(const std::string& msg, std::source_location loc);

// Modern type-safe fatal error. Logs to engine.log, dumps crash.log, shows MessageBox, exits.
template <typename... Args>
[[noreturn]] inline void fatalError(std::format_string<Args...> fmt, Args&&... args) {
    auto loc = std::source_location::current();
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    fatalErrorImpl(msg, loc);
}
