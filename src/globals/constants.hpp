#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace netbook::globals {

constexpr std::uint64_t print_delay_ms = 500;
constexpr std::uint64_t packet_creation_delay_ns = 0;
constexpr std::uint8_t dpdk_queue_count = 6;

// Write stats at this interval (higher = less frequent).
constexpr std::uint64_t write_stats_interval = 1000;

}