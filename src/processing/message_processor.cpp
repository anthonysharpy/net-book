#include <iostream>
#include "message_processor.hpp"
#include "types/types.hpp"
#include "globals/globals.hpp"

namespace netbook::processing {

using types::MarketMessage;

std::uint64_t packets_processed = 0;

void process_message(char* data, size_t) {
    auto message = MarketMessage::from_network_bytes(data);

    ++packets_processed;

    globals::packets_processed.store(packets_processed);
}

}
