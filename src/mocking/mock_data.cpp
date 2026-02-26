#include "mock_data.hpp"
#include "types/types.hpp"
#include "helpers/number_helpers.hpp"

namespace netbook::mocking {

using netbook::types::IncomingMarketMessage;

std::uint16_t current_index = 0;

IncomingMarketMessage create_mock_market_data() {
    return IncomingMarketMessage {
        .received_timestamp_nanoseconds = 0,
        .msg_id = ++current_index,
        .unit_price = static_cast<uint32_t>((helpers::get_random_number() % 1000) + 1),
        .index_id = static_cast<uint16_t>((helpers::get_random_number() % 19) + 1),
        .order_size = static_cast<uint16_t>((helpers::get_random_number() % 99) + 1),
    };
}

// Push mock data to the network controller.
void push_mock_data(std::stop_token stop) {
    while (!stop.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto message = create_mock_market_data();
        auto data = message.serialise_as_network_bytes();

        netbook::dpdk::push_data(reinterpret_cast<char*>(&data), sizeof(data));
    }
}

}