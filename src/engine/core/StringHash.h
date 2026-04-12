#pragma once
#include <functional>
#include <string>
#include <string_view>

// Transparent hash for std::unordered_map that enables heterogeneous lookup.
// Usage: std::unordered_map<std::string, V, StringHash, std::equal_to<>>
// This allows find() with std::string_view or const char* without constructing
// a temporary std::string.
struct StringHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};
