#include <iostream>
#include "message_processor.hpp"
#include "types/types.hpp"
#include "globals/globals.hpp"

namespace netbook::processing {

using types::MarketMessage;

void process_message(char* data, size_t) {
    [[maybe_unused]]
    auto message = MarketMessage::from_network_bytes(data);
}

}
