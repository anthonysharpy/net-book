#pragma once

#include <atomic>

namespace netbook::globals {

inline std::atomic<std::uint64_t> packets_written_to_dpdk = 0;
inline std::atomic<std::uint64_t> packets_read_from_dpdk = 0;
inline std::atomic<std::uint64_t> simulation_start_time_ns = 0;
inline std::uint64_t program_runtime_limit_seconds = 0;

}