#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace netbook::globals {

inline std::atomic<uint64_t> packets_sent = 0;
inline std::atomic<uint64_t> packets_processed = 0;
inline std::atomic<uint64_t> simulation_start_time_ns = 0;

}