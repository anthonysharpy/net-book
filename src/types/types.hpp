#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <cstring>
#include <rte_byteorder.h>
#include <string>
#include <sstream>
#include "helpers/time_helpers.hpp"

namespace netbook::types {

struct IncomingMarketMessage {
    std::uint32_t msg_id;
    std::uint32_t unit_price;
    std::uint16_t index_id;
    std::uint16_t order_size;

    std::array<char, 12> serialise_as_network_bytes() {
        std::array<char, 12> result;

        auto msg_id_reversed = rte_cpu_to_be_32(msg_id);
        auto unit_price_reversed = rte_cpu_to_be_32(unit_price);
        auto index_id_reversed = rte_cpu_to_be_16(index_id);
        auto order_size_reversed = rte_cpu_to_be_16(order_size);
        
        memcpy(&result[0], &msg_id_reversed, sizeof(msg_id_reversed));
        memcpy(&result[4], &unit_price_reversed, sizeof(unit_price_reversed));
        memcpy(&result[8], &index_id_reversed, sizeof(index_id_reversed));
        memcpy(&result[10], &order_size_reversed, sizeof(order_size_reversed));

        return result;
    }
};

// Largely the same as IncomingMarketMessage expect for internal use.
struct MarketMessage {
    std::uint64_t received_timestamp_nanoseconds;
    std::uint32_t msg_id;
    std::uint32_t unit_price;
    std::uint16_t index_id;
    std::uint16_t order_size;

    static MarketMessage from_network_bytes(char* data) {
        auto msg_id_reversed = reinterpret_cast<std::uint32_t*>(data);
        auto unit_price_reversed = reinterpret_cast<std::uint32_t*>(data + 4);
        auto index_id_reversed = reinterpret_cast<std::uint16_t*>(data + 8);
        auto order_size_reversed = reinterpret_cast<std::uint16_t*>(data + 10);

        return MarketMessage {
            .received_timestamp_nanoseconds = helpers::get_unix_timestamp_nanoseconds(),
            .msg_id = rte_be_to_cpu_32(*msg_id_reversed),
            .unit_price = rte_be_to_cpu_32(*unit_price_reversed),
            .index_id = rte_be_to_cpu_16(*index_id_reversed),
            .order_size = rte_be_to_cpu_16(*order_size_reversed),
        };
    }

    std::string to_string() {
        std::ostringstream oss;

        oss << "{" 
            << "received_timestamp_nanoseconds=" << received_timestamp_nanoseconds << ","
            << "msg_id=" << msg_id << ","
            << "unit_price=" << unit_price << ","
            << "index_id=" << index_id << ","
            << "order_size=" << order_size
            << "}";

        return oss.str();
    }
};

}