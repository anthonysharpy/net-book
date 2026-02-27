#pragma once

#include <random>

namespace netbook::helpers {

std::uint64_t get_unix_timestamp_nanoseconds();
std::uint64_t get_benchmark_timestamp_nanoseconds();

}