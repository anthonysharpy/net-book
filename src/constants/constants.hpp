#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include "types/types.hpp"

namespace netbook::constants {

constexpr std::uint32_t DPDK_QUEUE_COUNT = 8;
constexpr std::uint64_t PACKET_LIMIT = 2000000;
constexpr std::uint64_t PACKETS_PER_QUEUE = PACKET_LIMIT / DPDK_QUEUE_COUNT;
constexpr std::uint32_t PACKET_BATCH_SIZE = 32;
constexpr std::uint32_t MEMPOOL_SIZE = 8191;
constexpr size_t MARKET_MESSAGE_SIZE = sizeof(types::IncomingMarketMessage);

}