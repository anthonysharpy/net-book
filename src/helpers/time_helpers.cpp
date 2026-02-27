#include <random>
#include <chrono>
#include "time_helpers.hpp"

namespace netbook::helpers {

std::uint64_t get_unix_timestamp_nanoseconds() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

// A timestamp that can be used for benchmarking.
std::uint64_t get_benchmark_timestamp_nanoseconds() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

}