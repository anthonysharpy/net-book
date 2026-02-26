#include <random>
#include <chrono>
#include "time_helpers.hpp"

namespace netbook::helpers {

std::uint64_t get_unix_timestamp_nanoseconds() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

}