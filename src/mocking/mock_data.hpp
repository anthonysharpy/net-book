#pragma once

#include <dpdk/dpdk.hpp>
#include <cstdint>

namespace netbook::mocking {

void mock_data_pusher(std::uint8_t queue_id, std::uint32_t packets_to_push);

}
