#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "packet.hpp"
#include "logger.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <output_file>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string filename = argv[2];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_msg("ERROR", "Failed to open socket");
        return 1;
    }

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        log_msg("ERROR", "Bind failed");
        close(sockfd);
        return 1;
    }

    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile.is_open()) {
        log_msg("ERROR", "Could not open output file: " + filename);
        close(sockfd);
        return 1;
    }

    log_msg("INFO", "Receiver listening on port " + std::to_string(port) + "...");

    uint8_t network_buffer[HEADER_SIZE + MAX_PAYLOAD_SIZE];
    sockaddr_in src_addr{};
    socklen_t addr_len = sizeof(src_addr);

    bool transfer_active = true;

    while (transfer_active) {
        ssize_t bytes_received = recvfrom(sockfd, network_buffer, sizeof(network_buffer), 0,
                                          (struct sockaddr*)&src_addr, &addr_len);
        if (bytes_received < static_cast<ssize_t>(HEADER_SIZE)) {
            log_msg("WARN", "Corrupted or tiny fragments ignored.");
            continue;
        }

        // Validate Checksum 
        // 1. Snapshot original checksum field
        Packet extracted_pkt = deserialize_packet(network_buffer, bytes_received);
        uint16_t received_checksum = extracted_pkt.header.checksum;

        // 2. Zero out checksum location inside buffer to recompute validation value
        uint16_t zero_check = 0;
        std::memcpy(&network_buffer[10], &zero_check, 2);
        uint16_t local_checksum = compute_checksum(network_buffer, bytes_received);

        if (local_checksum != received_checksum) {
            log_msg("DROP", "Checksum mismatched! Packet dropped. Received: " + 
                    std::to_string(received_checksum) + ", Expected: " + std::to_string(local_checksum));
            continue;
        }

        if (extracted_pkt.header.flags & FLAG_FIN) {
            log_msg("RCVD", "FIN received. Closing file transmission smoothly.");
            transfer_active = false;
        } else if (extracted_pkt.header.flags & FLAG_DATA) {
            log_msg("RCVD", "Received Packet Seq: " + std::to_string(extracted_pkt.header.seq_num) + 
                    ", Length: " + std::to_string(extracted_pkt.header.length));
            
            outfile.write(extracted_pkt.payload, extracted_pkt.header.length);
        }
    }

    outfile.close();
    close(sockfd);
    return 0;
}