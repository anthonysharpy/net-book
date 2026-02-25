#include <stop_token>

namespace netbook::dpdk {

bool initialise();
void cleanup();
void poll(std::stop_token stop);

}