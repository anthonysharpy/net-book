#pragma once

#include <stop_token>
#include <vector>

namespace netbook::dpdk {

using DataCallbackSignature = void(*)(char*, size_t);

bool initialise();
void cleanup();
void poll_read(std::stop_token stop);
void poll_process_buffer(std::stop_token stop);
void push_data(char* data, size_t data_length);
void register_receiver(DataCallbackSignature callback);

}