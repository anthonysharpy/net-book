#include <cstdint>

namespace netbook::types {

struct IncomingMarketMessage {
    std::uint64_t received_timestamp_nanoseconds;
    // Assigned on receipt.
    std::uint32_t msg_id;
    std::uint32_t unit_price;
    std::uint16_t index_id;
    std::uint16_t order_size;
};

}