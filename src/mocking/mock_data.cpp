#include "mock_data.hpp"
#include "types/types.hpp"
#include "helpers/number_helpers.hpp"
#include "constants/constants.hpp"
#include "concurrency/concurrency.hpp"

namespace netbook::mocking {

using netbook::types::IncomingMarketMessage;

IncomingMarketMessage create_mock_market_data() {
    static std::uint16_t current_index = 0;

    return IncomingMarketMessage {
        .msg_id = ++current_index,
        .unit_price = static_cast<std::uint32_t>((helpers::get_random_number() % 1000) + 1),
        .index_id = static_cast<std::uint16_t>((helpers::get_random_number() % 19) + 1),
        .order_size = static_cast<std::uint16_t>((helpers::get_random_number() % 99) + 1),
    };
}

// Push mock data to the network controller.
void mock_data_pusher(std::stop_token stop, std::uint8_t queue_id, std::uint64_t packets_to_push) {
    concurrency::pin_thread_to_core(queue_id);

    while (!stop.stop_requested() && packets_to_push > 0) {
        #if packet_creation_delay_ns > 0
        std::this_thread::sleep_for(std::chrono::nanoseconds(constants::packet_creation_delay_ns));
        #endif

        auto message = create_mock_market_data();
        auto data = message.serialise_as_network_bytes();

        netbook::dpdk::push_data(reinterpret_cast<char*>(&data), sizeof(data), queue_id);
        --packets_to_push;
    }
}

}