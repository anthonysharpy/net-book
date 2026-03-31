#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace netbook::constants {

constexpr std::uint32_t dpdk_queue_count = 6;
constexpr std::uint64_t packet_limit = 1000000;
constexpr std::uint64_t packets_per_queue = packet_limit / dpdk_queue_count;
constexpr std::uint32_t packet_batch_size = 32;
constexpr std::uint32_t mempool_size = 8191;

}