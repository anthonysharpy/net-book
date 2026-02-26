#pragma once
#include <stop_token>
#include <dpdk.hpp>

namespace netbook::mocking {

void push_mock_data(std::stop_token stop);

}
