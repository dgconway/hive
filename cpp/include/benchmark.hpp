#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace bugs {

// Lightweight benchmarking system for profiling function execution times
class Benchmark {
public:
    struct TimingStats {
        int64_t total_ns = 0;
        int64_t call_count = 0;
        int64_t min_ns = std::numeric_limits<int64_t>::max();
        int64_t max_ns = 0;
    };

    static Benchmark& instance() {
        static Benchmark inst;
        return inst;
    }

    // RAII timer - automatically records duration when destroyed
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name) 
            : name_(name), start_(std::chrono::high_resolution_clock::now()) {}
        
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
            Benchmark::instance().record(name_, duration);
        }

    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
    };

    void record(const std::string& name, int64_t duration_ns) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& stats = timings_[name];
        stats.total_ns += duration_ns;
        stats.call_count++;
        stats.min_ns = std::min(stats.min_ns, duration_ns);
        stats.max_ns = std::max(stats.max_ns, duration_ns);
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        timings_.clear();
    }

    std::string report() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream ss;
        ss << "\n=== BENCHMARK REPORT ===\n";
        ss << std::left << std::setw(30) << "Function"
           << std::right << std::setw(12) << "Calls"
           << std::setw(15) << "Total (ms)"
           << std::setw(15) << "Avg (us)"
           << std::setw(12) << "Min (us)"
           << std::setw(12) << "Max (us)" << "\n";
        ss << std::string(96, '-') << "\n";

        // Sort by total time descending
        std::vector<std::pair<std::string, TimingStats>> sorted(timings_.begin(), timings_.end());
        std::sort(sorted.begin(), sorted.end(), 
            [](const auto& a, const auto& b) { return a.second.total_ns > b.second.total_ns; });

        for (const auto& [name, stats] : sorted) {
            double total_ms = stats.total_ns / 1e6;
            double avg_us = stats.call_count > 0 ? (stats.total_ns / stats.call_count) / 1e3 : 0;
            double min_us = stats.min_ns / 1e3;
            double max_us = stats.max_ns / 1e3;

            ss << std::left << std::setw(30) << name
               << std::right << std::setw(12) << stats.call_count
               << std::setw(15) << std::fixed << std::setprecision(2) << total_ms
               << std::setw(15) << std::fixed << std::setprecision(2) << avg_us
               << std::setw(12) << std::fixed << std::setprecision(2) << min_us
               << std::setw(12) << std::fixed << std::setprecision(2) << max_us << "\n";
        }
        ss << "========================\n";
        return ss.str();
    }

    const std::unordered_map<std::string, TimingStats>& get_timings() const {
        return timings_;
    }

private:
    Benchmark() = default;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TimingStats> timings_;
};

// Convenience macro for benchmarking a scope
#define BENCHMARK_SCOPE(name) bugs::Benchmark::ScopedTimer _timer_##__LINE__(name)

// Macro to benchmark and log only in debug builds
#ifdef NDEBUG
    #define BENCHMARK_SCOPE_DEBUG(name) ((void)0)
#else
    #define BENCHMARK_SCOPE_DEBUG(name) BENCHMARK_SCOPE(name)
#endif

} // namespace bugs
