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
#include <cstddef>
#include "dpdk.hpp"
#include "concurrency/spscringbuffer.hpp"

// Some code taken from https://github.com/awaiskhalidawan/dpdk-tutorials.
namespace netbook::dpdk {

// Combined length of Ethernet, IPv4 and UDP headers.
static constexpr size_t headers_length = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr);
static constexpr size_t write_buffer_size = 128;
static constexpr size_t read_buffer_size = 128;

struct WriteRequest {
    char* location;
    size_t size;
};

// Data is pushed here before writing.
netbook::concurrency::SPSCRingBuffer<WriteRequest, write_buffer_size> write_buffer{};

// Data is pushed here after reading (but before processing).
netbook::concurrency::SPSCRingBuffer<rte_mbuf*, read_buffer_size> read_buffer{};

// When we process data, we pass it to the provided callback functions.
inline std::vector<DataCallbackSignature> callback_functions;

// For now we'll just hard-code the pool and port here.
rte_mempool* mempool;
int port_id = -1;

void register_receiver(DataCallbackSignature callback) {
    std::cout << "Registering network callback receiver..." << std::endl;

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

// Push data into the buffer for writing.
void push_data(char* data, size_t data_length) {
    write_buffer.push(WriteRequest{data, data_length});
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

// Poll (read) the already created Ethernet device and store incoming packets in the read buffer.
void poll_read(std::stop_token stop) {
    rte_mbuf *received_packets[32];
    std::uint16_t packets_count = 0;

    while (!stop.stop_requested()) {
        packets_count = rte_eth_rx_burst(port_id, 0, received_packets, 32);

        if (packets_count == 0) {
            continue;
        }

        for (int i = 0; i < packets_count; ++i) {
            read_buffer.push(received_packets[i]);
        }
    }
}

// Poll the read buffer, sending packets out to be processed.
void poll_read_buffer(std::stop_token stop) {
    rte_mbuf* data[32];
    size_t items_popped = 0;

    while (!stop.stop_requested()) {
        items_popped = read_buffer.pop_many(data, 32);

        for (int i = 0; i < items_popped; ++i) {
            auto data_location = rte_pktmbuf_mtod(data[i], char*);
            auto inner_data_location = data_location + headers_length;
            auto inner_data_length = data[i]->data_len - headers_length;

            for (const DataCallbackSignature& callback : callback_functions) {
                callback(inner_data_location, inner_data_length);
            }
        }

        rte_pktmbuf_free_bulk(data, items_popped);
    }
}

rte_mbuf* create_packet(char* data, std::size_t data_length) {
    rte_mbuf* packet = nullptr;

    // Get a memory buffer from our memory pool. On this memory buffer we will write our packet data.
    if (rte_mempool_get(mempool, reinterpret_cast<void **>(&packet)) != 0) {
        std::cout << "Unable to get memory buffer from memory pool" << std::endl;
        return nullptr;
    }

    // We have successfully got the memory buffer. Now we will write the packet data. A memory buffer in DPDK is divided 
    // into parts i.e. Head room memory area, main memory area and tail room memory area. The details are available at:
    // https://doc.dpdk.org/guides/prog_guide/mbuf_lib.html
    // We will get a pointer to the main memory area of our memory buffer and write packet info.
    uint8_t *mbuf_data = rte_pktmbuf_mtod(packet, uint8_t *);

    // Setting Ethernet header information (Source MAC, Destination MAC, Ethernet type).
    rte_ether_hdr *const eth_hdr = reinterpret_cast<rte_ether_hdr *>(mbuf_data);
    eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    const uint8_t src_mac_addr[6] = {0x08, 0x00, 0x27, 0x95, 0xBD, 0xAE};
    memcpy(eth_hdr->src_addr.addr_bytes, src_mac_addr, sizeof(src_mac_addr));

    const uint8_t dst_mac_addr[6] = {0x08, 0x00, 0x27, 0x35, 0x14, 0x15};
    memcpy(eth_hdr->dst_addr.addr_bytes, dst_mac_addr, sizeof(dst_mac_addr));

    // Setting IPv4 header information.
    rte_ipv4_hdr *const ipv4_hdr = reinterpret_cast<rte_ipv4_hdr *>(mbuf_data + sizeof(rte_ether_hdr));
    ipv4_hdr->version = 4;              // Setting IP version as IPv4
    ipv4_hdr->ihl = 5;                  // Setting IP header length = 20 bytes = (5 * 4 Bytes)
    ipv4_hdr->type_of_service = 0;      // Setting DSCP = 0; ECN = 0;
    ipv4_hdr->total_length = rte_cpu_to_be_16(20 + 8 + data_length); // Including 20 byte IPv4 header and 8 byte UDP header.
    ipv4_hdr->packet_id = 0;            // Setting identification = 0 as the packet is non-fragmented.
    ipv4_hdr->fragment_offset = 0x0040; // Setting packet as non-fragmented and fragment offset = 0.
    ipv4_hdr->time_to_live = 64;        // Setting Time to live = 64;
    ipv4_hdr->next_proto_id = 17;       // Setting the next protocol as UDP (17).

    const uint8_t src_ip_addr[4] = {10, 0, 9, 8};
    memcpy(&ipv4_hdr->src_addr, src_ip_addr, sizeof(src_ip_addr));      // Setting source ip address = 1.2.3.4

    const uint8_t dest_ip_addr[4] = {10, 0, 9, 6};
    memcpy(&ipv4_hdr->dst_addr, dest_ip_addr, sizeof(dest_ip_addr));    // Setting destination ip address = 4.3.2.1

    ipv4_hdr->hdr_checksum = 0;
    ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);      // Calculating and setting IPv4 checksum in IPv4 header.

    // Setting UDP header information.
    rte_udp_hdr *const udp_hdr = reinterpret_cast<rte_udp_hdr *>(mbuf_data + sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr));
    udp_hdr->dst_port = rte_cpu_to_be_16(5000);     // Setting destination port = 5000
    udp_hdr->src_port = rte_cpu_to_be_16(10000);    // Setting source port = 10000
    udp_hdr->dgram_len = rte_cpu_to_be_16(8 + data_length); // Including 8 byte UDP header
    udp_hdr->dgram_cksum = 0;                       // Setting checksum = 0

    // Setting data in the UDP payload
    uint8_t *payload_location = mbuf_data + sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr);
    memset(payload_location, 0, data_length);
    memcpy(payload_location, data, data_length);

    // Setting the total packet size in our memory buffer.
    // Total packet size = Ethernet header size + IPv4 header size + UDP header size + Payload size.
    packet->data_len = packet->pkt_len = headers_length + data_length;

    return packet;
}

void send_packet(rte_mbuf* packet) {
    // The DPDK API `rte_eth_tx_burst` will automatically release the memory buffer after tranmission is successful.
    const uint16_t packets_sent = rte_eth_tx_burst(port_id, 0, &packet, 1);
    
    if (packets_sent == 0) {
        std::cout << "Unable to transmit the packet" << std::endl;
        rte_pktmbuf_free(packet);   // As the packet is not transmitted, we need to free the memory buffer by our self.
    }
}

// Poll (write) the already created Ethernet device.
void poll_write(std::stop_token stop) {
    WriteRequest data[write_buffer_size];

    while (!stop.stop_requested()) {
        auto items_popped = write_buffer.pop_many(data, write_buffer_size);
        
        for (size_t i = 0; i < items_popped; ++i) {
            auto packet = create_packet(data[i].location, data[i].size);
            send_packet(packet);
        }
    }
}

}