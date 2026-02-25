#include <rte_eal.h>
#include <cerrno>
#include <iostream>
#include <rte_mbuf.h>      
#include <rte_mempool.h>   
#include <rte_errno.h>     
#include <rte_lcore.h>    
#include <rte_ethdev.h>
#include <vector>
#include <cstdint>
#include <thread>
#include <stop_token>

// Some code taken from https://github.com/awaiskhalidawan/dpdk-tutorials.
namespace netbook::dpdk {

// For now we'll just hard-code the pool and port here.
rte_mempool* mempool;
int port_id = -1;

void cleanup() {
    if (port_id != -1) {
        rte_eth_dev_stop(port_id);
        rte_eth_dev_close(port_id);
    }
    
    if (mempool != nullptr) {
        rte_mempool_free(mempool);
    }

    int status = rte_eal_cleanup();

     if (status != 0) {
        std::cerr << "Failed cleaning up DPDK: " << status << std::endl;
    }

    std::cout << "DPDK cleaned up successfuly" << std::endl;
}

// Returns true on success.
bool setup_receive_queues() {
    int status = rte_eth_rx_queue_setup(port_id, 0, 256, SOCKET_ID_ANY, nullptr, mempool);
    
    if (status != 0) {
        std::cerr << "Failed setting up receive queue: " << status << std::endl;
        return false;
    }

    return true;
}

// Returns true on success.
bool setup_transmit_queues() {
    int status = rte_eth_tx_queue_setup(port_id, 0, 256, SOCKET_ID_ANY, nullptr);
    
    if (status != 0) {
        std::cerr << "Failed setting up transmit queue: " << status << std::endl;
        return false;
    }

    return true;
}

std::vector<int> get_port_ids() {
    std::vector<int> port_ids;
    int port_id;

    RTE_ETH_FOREACH_DEV(port_id) {
        port_ids.push_back(port_id);
    }

    return port_ids;
}

// Returns true on success.
bool initialise() {
    std::cout << "Initialising DPDK..." << std::endl;

    char* eal_args[] = {
        (char*)"netbook", 
        (char*)"--vdev=net_ring0",
        (char*)"--vdev=net_ring1",
        (char*)"--no-pci", 
        (char*)"--log-level=eal,debug", 
        (char*)"-l", 
        (char*)"0-1", 
        (char*)"-n", 
        (char*)"4", 
        nullptr
    };
    int status = rte_eal_init(9, eal_args);

    if (status < 0) {
        std::cerr << "Failed initialising DPDK: " << status << std::endl;
        cleanup();
        return false;
    }

    std::cout << "Creating mbuf pool..." << std::endl;

    mempool = rte_pktmbuf_pool_create("netbook-pool", 1023, 128, 0, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY);

    if (mempool == nullptr) {
        std::cerr << "Failed initialising mbuf pool" << std::endl;
        cleanup();
        return false;
    }

    auto port_ids = get_port_ids();

    if (port_ids.size() == 0) {
        std::cerr << "No port IDs found" << std::endl;
        cleanup();
        return false;
    }

    port_id = port_ids[0];

    rte_eth_conf port_configuration = {
        .rxmode = {
            .mq_mode = RTE_ETH_MQ_RX_NONE
        },
        .txmode = {
            .mq_mode = RTE_ETH_MQ_TX_NONE
        }
    };

    std::cout << "Configuring Ethernet device..." << std::endl;

    status = rte_eth_dev_configure(port_id, 1, 1, &port_configuration);

    if (status != 0) {
        std::cerr << "Unable to configure Ethernet device: " << status << std::endl;
        cleanup();
        return false;
    }

    std::cout << "Setting up receive queues..." << std::endl;

    if (!setup_receive_queues()) {
        std::cerr << "Failed setting up receive queues" << std::endl;
        cleanup();
        return false;
    }

    std::cout << "Setting up transmit queues..." << std::endl;

    if (!setup_transmit_queues()) {
        std::cerr << "Failed setting up transmit queues" << std::endl;
        cleanup();
        return false;
    }

    std::cout << "Trying to enable promiscuous mode..." << std::endl;

    // This might fail as this is supposedly not supported everywhere.
    rte_eth_promiscuous_enable(port_id);

    std::cout << "Starting ethernet device..." << std::endl;

    status = rte_eth_dev_start(port_id);

    if (status < 0) {
        std::cout << "Failed to start the Ethernet device: " << status << std::endl;
        cleanup();
        return false;
    }

    return true;
}

// Poll the already created Ethernet device.
void poll(std::stop_token stop) {
    rte_mbuf *received_packets[32];
    std::uint16_t packets_count = 0;

    while (!stop.stop_requested()) {
        packets_count = rte_eth_rx_burst(port_id, 0, received_packets, 32);

        if (packets_count == 0) {
            //std::this_thread::sleep_for(std::chrono::microseconds(10));
            continue;
        }

        for (int i = 0; i < packets_count; ++i) {
            std::cout << "Received packet of length " << received_packets[i]->data_len << std::endl;
        }

        rte_pktmbuf_free_bulk(received_packets, packets_count);
    }
}

}