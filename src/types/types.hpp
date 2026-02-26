#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <cstring>
#include <rte_byteorder.h>

namespace netbook::types {

struct IncomingMarketMessage {
    std::uint64_t received_timestamp_nanoseconds;
    // Assigned on receipt.
    std::uint32_t msg_id;
    std::uint32_t unit_price;
    std::uint16_t index_id;
    std::uint16_t order_size;

    std::array<char, 20> serialise_as_network_bytes(IncomingMarketMessage message) {
        std::array<char, 20> result;

        auto received_timestamp_nanoseconds_reversed = rte_cpu_to_be_64(received_timestamp_nanoseconds);
        auto msg_id_reversed = rte_cpu_to_be_32(msg_id);
        auto unit_price_reversed = rte_cpu_to_be_32(unit_price);
        auto index_id_reversed = rte_cpu_to_be_16(index_id);
        auto order_size_reversed = rte_cpu_to_be_16(order_size);
        
        memcpy(&result[0], &received_timestamp_nanoseconds_reversed, received_timestamp_nanoseconds);
        memcpy(&result[4], &msg_id_reversed, msg_id);
        memcpy(&result[6], &unit_price_reversed, unit_price);
        memcpy(&result[8], &index_id_reversed, index_id);
        memcpy(&result[9], &order_size_reversed, order_size);

        return result;
    }
};

}