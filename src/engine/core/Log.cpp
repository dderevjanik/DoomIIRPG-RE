#include "Log.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

// --- Log file sink ---

static FILE* g_logFile = nullptr;

// --- Ring buffer for recent log entries (crash context) ---

static constexpr size_t RING_SIZE = 128;
static std::array<std::string, RING_SIZE> g_ring;
static std::atomic<size_t> g_ringIndex{0};
static std::mutex g_ringMutex; // protects g_ring writes + file writes

void logInit(const char* path) {
    g_logFile = std::fopen(path, "w");
    if (!g_logFile) {
        std::print(stderr, "[LOG] Warning: could not open log file '{}'\n", path);
    }
}

void logShutdown() {
    if (g_logFile) {
        std::fflush(g_logFile);
        std::fclose(g_logFile);
        g_logFile = nullptr;
    }
}

void logEmit(LogLevel level, const std::string& line) {
    // Write to stderr
    std::print(stderr, "{}", line);

    std::lock_guard<std::mutex> lock(g_ringMutex);

    // Write to log file
    if (g_logFile) {
        std::fwrite(line.data(), 1, line.size(), g_logFile);
        // Flush on WARN/ERROR for reliability
        if (level >= LOG_LEVEL_WARN) {
            std::fflush(g_logFile);
        }
    }

    // Store in ring buffer (strip trailing newline for cleaner crash.log)
    std::string entry = line;
    while (!entry.empty() && entry.back() == '\n') {
        entry.pop_back();
    }
    size_t idx = g_ringIndex.fetch_add(1, std::memory_order_relaxed) % RING_SIZE;
    g_ring[idx] = std::move(entry);
}

std::vector<std::string> logGetRecentEntries(int count) {
    if (count <= 0) return {};
    if (count > (int)RING_SIZE) count = (int)RING_SIZE;

    std::vector<std::string> result;
    result.reserve(count);

    size_t current = g_ringIndex.load(std::memory_order_relaxed);
    size_t total = std::min((size_t)count, current); // don't return more than written

    // Walk backwards from most recent
    for (size_t i = 0; i < total; i++) {
        size_t idx = (current - 1 - i) % RING_SIZE;
        if (!g_ring[idx].empty()) {
            result.push_back(g_ring[idx]);
        }
    }

    // Reverse so entries are in chronological order
    std::reverse(result.begin(), result.end());
    return result;
}

void logDumpRecentToFile(FILE* f, int count) {
    auto entries = logGetRecentEntries(count);
    if (entries.empty()) return;

    fprintf(f, "\n=== RECENT LOG (%d entries) ===\n", (int)entries.size());
    for (const auto& entry : entries) {
        fprintf(f, "  %s\n", entry.c_str());
    }
}
