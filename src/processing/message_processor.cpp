#include <iostream>
#include "message_processor.hpp"
#include "types/types.hpp"

namespace netbook::processing {

using types::MarketMessage;

void process_message(char* data, size_t) {
    auto message = MarketMessage::from_network_bytes(data);

    std::cout << "Received a message: " << message.to_string() << std::endl;
}

}
