#pragma once

#include <stop_token>
#include <dpdk/dpdk.hpp>

namespace netbook::mocking {

void mock_data_pusher(std::stop_token stop, std::uint8_t queue_id);

}
