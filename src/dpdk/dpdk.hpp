#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <array>
#include "constants/constants.hpp"

namespace netbook::dpdk {

using DataCallbackSignature = void(*)(char*, size_t);

bool initialise();
void cleanup();
void poll_read(std::uint8_t queue_id, std::uint64_t packets_to_read);
void push_data(std::uint32_t packet_count, std::array<std::array<char, 12>, constants::packet_batch_size>& data, std::array<size_t, constants::packet_batch_size>& data_lengths, std::uint8_t queue_id);
void register_receiver(DataCallbackSignature callback);

}