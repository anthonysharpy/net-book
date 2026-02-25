#include <rte_eal.h>
#include <cerrno>
#include <iostream>
#include <rte_mbuf.h>      
#include <rte_mempool.h>   
#include <rte_errno.h>     
#include <rte_lcore.h>    

namespace netbook::dpdk {

void cleanup() {
    int status = rte_eal_cleanup();

     if (status != 0) {
        std::cerr << "Failed cleaning up DPDK: " << status << std::endl;
    }
}

// Returns true on success.
bool initialise() {
    std::cout << "Initialising DPDK..." << std::endl;

    int status = rte_eal_init(0, nullptr);

    if (status != 0) {
        std::cerr << "Failed initialising DPDK: " << status << std::endl;
        cleanup();
        return false;
    }

    auto mempool = rte_pktmbuf_pool_create("netbook-pool", 1023, 128, 0, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY);

    if (mempool == nullptr) {
        std::cerr << "Failed initialising mbuf pool" << std::endl;
        cleanup();
        return false;
    }

    return true;
}

}