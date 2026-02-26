#pragma once

#include <cstddef>

// Takes incoming network messages and processes them.
namespace netbook::processing {

void process_message(char* data, size_t length);

}
