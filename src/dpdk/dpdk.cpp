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
#include <cstddef>
#include "dpdk.hpp"
#include "concurrency/spscringbuffer.hpp"
#include "constants/constants.hpp"
#include "concurrency/concurrency.hpp"

// Some code taken from https://github.com/awaiskhalidawan/dpdk-tutorials.
namespace netbook::dpdk {

// Combined length of Ethernet, IPv4 and UDP headers.
constexpr size_t HEADERS_LENGTH = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr);

// When we process data, we pass it to the provided callback functions.
inline std::vector<DataCallbackSignature> callback_functions;

// For now we'll just hard-code the pool and port here.
rte_mempool* mempool;
int port_id = -1;

void register_receiver(DataCallbackSignature callback) {
    std::cout << "Registering network callback receiver...\n";

    callback_functions.push_back(callback);
}

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
        std::cerr << "Failed cleaning up DPDK: " << status << "\n";
    }

    std::cout << "DPDK cleaned up successfuly\n";
}

// Returns true on success.
bool setup_receive_queues() {
    for (unsigned int i = 0; i < constants::DPDK_QUEUE_COUNT; ++i) {
        int status = rte_eth_rx_queue_setup(port_id, i, 128, SOCKET_ID_ANY, nullptr, mempool);
        
        if (status != 0) {
            std::cerr << "Failed setting up receive queue: " << status << "\n";
            return false;
        }
    }

    return true;
}

// Returns true on success.
bool setup_transmit_queues() {
    for (unsigned int i = 0; i < constants::DPDK_QUEUE_COUNT; ++i) {
        int status = rte_eth_tx_queue_setup(port_id, i, 128, SOCKET_ID_ANY, nullptr);
        
        if (status != 0) {
            std::cerr << "Failed setting up transmit queue: " << status << "\n";
            return false;
        }
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
    std::cout << "Initialising DPDK...\n";

    std::string core_range = "0-" + std::to_string(constants::DPDK_QUEUE_COUNT - 1);

    char* eal_args[] = {
        (char*)"netbook", 
        (char*)"--vdev=net_ring0",
        (char*)"--vdev=net_ring1",
        (char*)"--no-pci", 
        (char*)"--log-level=eal,debug", 
        (char*)"-l", 
        core_range.data(), 
        (char*)"-n", 
        (char*)"4", 
        nullptr
    };
    int status = rte_eal_init(9, eal_args);

    if (status < 0) {
        std::cerr << "Failed initialising DPDK: " << status << "\n";
        cleanup();
        return false;
    }

    std::cout << "Creating mbuf pool...\n";

    mempool = rte_pktmbuf_pool_create(
        "netbook-pool",
        constants::MEMPOOL_SIZE,
        128,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        SOCKET_ID_ANY
    );

    if (mempool == nullptr) {
        std::cerr << "Failed initialising mbuf pool: " << rte_errno << "\n";
        cleanup();
        return false;
    }

    auto port_ids = get_port_ids();

    if (port_ids.size() == 0) {
        std::cerr << "No port IDs found\n";
        cleanup();
        return false;
    }

    port_id = port_ids[0];

    rte_eth_conf port_configuration{};
    port_configuration.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;
    port_configuration.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    std::cout << "Configuring Ethernet device...\n";

    status = rte_eth_dev_configure(port_id, constants::DPDK_QUEUE_COUNT, constants::DPDK_QUEUE_COUNT, &port_configuration);

    if (status != 0) {
        std::cerr << "Unable to configure Ethernet device: " << status << "\n";
        cleanup();
        return false;
    }

    std::cout << "Setting up receive queues...\n";

    if (!setup_receive_queues()) {
        std::cerr << "Failed setting up receive queues\n";
        cleanup();
        return false;
    }

    std::cout << "Setting up transmit queues...\n";

    if (!setup_transmit_queues()) {
        std::cerr << "Failed setting up transmit queues\n";
        cleanup();
        return false;
    }

    std::cout << "Trying to enable promiscuous mode...\n";

    // This might fail as this is supposedly not supported everywhere.
    rte_eth_promiscuous_enable(port_id);

    std::cout << "Starting ethernet device...\n";

    status = rte_eth_dev_start(port_id);

    if (status < 0) {
        std::cout << "Failed to start the Ethernet device: " << status << "\n";
        cleanup();
        return false;
    }

    return true;
}

// Poll (read) the already created Ethernet device and process the incoming packets.
void poll_read(std::uint8_t queue_id, std::uint64_t packets_to_read) {
    concurrency::pin_thread_to_core(queue_id);

    std::array<rte_mbuf*, constants::PACKET_BATCH_SIZE> received_packets;
    std::uint16_t packets_count = 0;

    while (packets_to_read > 0) {
        packets_count = rte_eth_rx_burst(port_id, queue_id, received_packets.data(), constants::PACKET_BATCH_SIZE);

        if (packets_count == 0) {
            continue;
        }

        for (int i = 0; i < packets_count; ++i) {
            auto data_location = rte_pktmbuf_mtod(received_packets[i], char*);
            auto inner_data_location = data_location + HEADERS_LENGTH;
            auto inner_data_length = received_packets[i]->data_len - HEADERS_LENGTH;

            for (const DataCallbackSignature& callback : callback_functions) {
                callback(inner_data_location, inner_data_length);
            }
        }

        rte_pktmbuf_free_bulk(received_packets.data(), packets_count);

        packets_to_read -= packets_count;
    }
}

bool create_packets(
    std::uint32_t packet_count,
    std::array<std::array<char, 12>, constants::PACKET_BATCH_SIZE>& data,
    std::array<size_t, constants::PACKET_BATCH_SIZE>& data_lengths,
    std::array<rte_mbuf*, constants::PACKET_BATCH_SIZE>& packets_out
) {
    // Get a memory buffer from our memory pool. On this memory buffer we will write our packet data.
    if (rte_mempool_get_bulk(mempool, reinterpret_cast<void**>(packets_out.data()), packet_count) != 0) {
        // Maybe there is no space in the mempool.
        return false;
    }

    for (unsigned int i = 0; i < packet_count; i++) {
        // We have successfully got the memory buffer. Now we will write the packet data. A memory buffer in DPDK is divided 
        // into parts i.e. Head room memory area, main memory area and tail room memory area. The details are available at:
        // https://doc.dpdk.org/guides/prog_guide/mbuf_lib.html
        // We will get a pointer to the main memory area of our memory buffer and write packet info.
        std::uint8_t* mbuf_data = rte_pktmbuf_mtod(packets_out[i], std::uint8_t *);

        // Setting Ethernet header information (Source MAC, Destination MAC, Ethernet type).
        rte_ether_hdr *const eth_hdr = reinterpret_cast<rte_ether_hdr *>(mbuf_data);
        eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

        const std::uint8_t src_mac_addr[6] = {0x08, 0x00, 0x27, 0x95, 0xBD, 0xAE};
        memcpy(eth_hdr->src_addr.addr_bytes, src_mac_addr, sizeof(src_mac_addr));

        const std::uint8_t dst_mac_addr[6] = {0x08, 0x00, 0x27, 0x35, 0x14, 0x15};
        memcpy(eth_hdr->dst_addr.addr_bytes, dst_mac_addr, sizeof(dst_mac_addr));

        // Setting IPv4 header information.
        rte_ipv4_hdr *const ipv4_hdr = reinterpret_cast<rte_ipv4_hdr *>(mbuf_data + sizeof(rte_ether_hdr));
        ipv4_hdr->version = 4;              // Setting IP version as IPv4
        ipv4_hdr->ihl = 5;                  // Setting IP header length = 20 bytes = (5 * 4 Bytes)
        ipv4_hdr->type_of_service = 0;      // Setting DSCP = 0; ECN = 0;
        ipv4_hdr->total_length = rte_cpu_to_be_16(20 + 8 + data_lengths[i]); // Including 20 byte IPv4 header and 8 byte UDP header.
        ipv4_hdr->packet_id = 0;            // Setting identification = 0 as the packet is non-fragmented.
        ipv4_hdr->fragment_offset = 0x0040; // Setting packet as non-fragmented and fragment offset = 0.
        ipv4_hdr->time_to_live = 64;        // Setting Time to live = 64;
        ipv4_hdr->next_proto_id = 17;       // Setting the next protocol as UDP (17).

        const std::uint8_t src_ip_addr[4] = {10, 0, 9, 8};
        memcpy(&ipv4_hdr->src_addr, src_ip_addr, sizeof(src_ip_addr));      // Setting source ip address = 1.2.3.4

        const std::uint8_t dest_ip_addr[4] = {10, 0, 9, 6};
        memcpy(&ipv4_hdr->dst_addr, dest_ip_addr, sizeof(dest_ip_addr));    // Setting destination ip address = 4.3.2.1

        ipv4_hdr->hdr_checksum = 0;
        ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);      // Calculating and setting IPv4 checksum in IPv4 header.

        // Setting UDP header information.
        rte_udp_hdr *const udp_hdr = reinterpret_cast<rte_udp_hdr *>(mbuf_data + sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr));
        udp_hdr->dst_port = rte_cpu_to_be_16(5000);     // Setting destination port = 5000
        udp_hdr->src_port = rte_cpu_to_be_16(10000);    // Setting source port = 10000
        udp_hdr->dgram_len = rte_cpu_to_be_16(8 + data_lengths[i]); // Including 8 byte UDP header
        udp_hdr->dgram_cksum = 0;                       // Setting checksum = 0

        // Setting data in the UDP payload
        std::uint8_t *payload_location = mbuf_data + sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr);
        memset(payload_location, 0, data_lengths[i]);
        memcpy(payload_location, data[i].data(), data_lengths[i]);

        // Setting the total packet size in our memory buffer.
        // Total packet size = Ethernet header size + IPv4 header size + UDP header size + Payload size.
        packets_out[i]->data_len = packets_out[i]->pkt_len = HEADERS_LENGTH + data_lengths[i];
    }

    return true;
}

void send_packets(
    std::uint32_t packet_count,
    std::array<rte_mbuf*, constants::PACKET_BATCH_SIZE>& packets,
    std::uint8_t queue_id
) {
    // The DPDK API `rte_eth_tx_burst` will automatically release the memory buffer after
    // tranmission is successful.
    unsigned int progress = 0;

    while (progress < packet_count) {
        progress += rte_eth_tx_burst(port_id, queue_id, packets.data() + progress, packet_count-progress);
    }
}

// Push data into the buffer for writing.
void push_data(
    std::uint32_t packet_count,
    std::array<std::array<char, 12>, constants::PACKET_BATCH_SIZE>& data,
    std::array<size_t, constants::PACKET_BATCH_SIZE>& data_lengths,
    std::uint8_t queue_id
) {
    std::array<rte_mbuf*, constants::PACKET_BATCH_SIZE> packets;

    // Keep retrying until we can successfully create it.
    while (!create_packets(packet_count, data, data_lengths, packets)) {}

    send_packets(packet_count, packets, queue_id);
}

}