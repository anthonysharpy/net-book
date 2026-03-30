#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace netbook::constants {

constexpr std::uint64_t print_delay_ms = 500;
constexpr std::uint64_t packet_creation_delay_ns = 0;
constexpr std::uint32_t dpdk_queue_count = 6;
constexpr std::uint64_t packet_limit = 10000000;
constexpr std::uint64_t packets_per_queue = packet_limit / dpdk_queue_count;

}