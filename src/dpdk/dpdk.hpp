#include <stop_token>

namespace netbook::dpdk {

bool initialise();
void cleanup();
void poll_write(std::stop_token stop);
void poll_read(std::stop_token stop);

}