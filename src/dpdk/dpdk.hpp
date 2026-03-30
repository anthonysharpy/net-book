#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace netbook::dpdk {

using DataCallbackSignature = void(*)(char*, size_t);

bool initialise();
void cleanup();
void poll_read(std::uint8_t queue_id, std::uint64_t packets_to_read);
void push_data(char* data, size_t data_length, std::uint8_t queue_id);
void register_receiver(DataCallbackSignature callback);

}