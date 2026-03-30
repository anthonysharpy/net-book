#pragma once

#include <stop_token>

namespace netbook::output {

void print_stats_thread(std::stop_token stop);

}