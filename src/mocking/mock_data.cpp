#include "mock_data.hpp"
#include "types/types.hpp"
#include "helpers/number_helpers.hpp"
#include "constants/constants.hpp"
#include "concurrency/concurrency.hpp"

namespace netbook::mocking {

using netbook::types::IncomingMarketMessage;

void create_mock_market_data(
    int count,
    std::array<IncomingMarketMessage, constants::PACKET_BATCH_SIZE> &data_out
) { 
    static std::uint16_t current_index = 0;

    --count;

    while (count >= 0) {
        data_out[count] = IncomingMarketMessage {
            .msg_id = ++current_index,
            .unit_price = static_cast<std::uint32_t>((helpers::get_random_number() % 1000) + 1),
            .index_id = static_cast<std::uint16_t>((helpers::get_random_number() % 19) + 1),
            .order_size = static_cast<std::uint16_t>((helpers::get_random_number() % 99) + 1),
        };

        --count;
    }    
}

// Push mock data to the network controller.
void mock_data_pusher(std::uint8_t queue_id, std::uint32_t packets_to_push) {
    concurrency::pin_thread_to_core(queue_id);

    std::array<IncomingMarketMessage, constants::PACKET_BATCH_SIZE> messages;
    std::array<std::array<char, 12>, constants::PACKET_BATCH_SIZE> messages_as_bytes;
    std::array<size_t, constants::PACKET_BATCH_SIZE> message_lengths;

    while (packets_to_push > 0) {
        unsigned int packet_count = std::min(packets_to_push, constants::PACKET_BATCH_SIZE);

        create_mock_market_data(packet_count, messages);

        for (unsigned int i = 0; i < packet_count; i++) {
            messages_as_bytes[i] = messages[i].serialise_as_network_bytes();
            message_lengths[i] = sizeof(messages_as_bytes[i]);
        }

        netbook::dpdk::push_data(packet_count, messages_as_bytes, message_lengths, queue_id);

        packets_to_push -= packet_count;
    }
}

}