#include <stop_token>

namespace netbook::dpdk {

int initialise();
void cleanup();
void poll(std::stop_token stop, int port_id);

}