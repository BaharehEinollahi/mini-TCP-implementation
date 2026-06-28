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

    log_msg("INFO", "Receiver active on port " + std::to_string(port) + ". Expecting data...");

    uint8_t network_buffer[HEADER_SIZE + MAX_PAYLOAD_SIZE];
    sockaddr_in src_addr{};
    socklen_t addr_len = sizeof(src_addr);

    uint32_t expected_seq_num = 0; 
    bool running = true;

    while (running) {
        ssize_t bytes_recvd = recvfrom(sockfd, network_buffer, sizeof(network_buffer), 0,
                                         (struct sockaddr*)&src_addr, &addr_len);
        if (bytes_recvd < static_cast<ssize_t>(HEADER_SIZE)) continue;

        // Verify Checksum Integrity
        Packet pkt = deserialize_packet(network_buffer, bytes_recvd);
        uint16_t received_chk = pkt.header.checksum;
        uint16_t zero_chk = 0;
        std::memcpy(&network_buffer[10], &zero_chk, 2);
        uint16_t computed_chk = compute_checksum(network_buffer, bytes_recvd);

        if (received_chk != computed_chk) {
            log_msg("DROP", "Bad Checksum. Drop frame.");
            continue;
        }

        if (pkt.header.flags & FLAG_DATA) {
            if (pkt.header.seq_num == expected_seq_num) {
                // In-order packet arrived
                log_msg("RCVD", "Accepting Packet. Seq: " + std::to_string(pkt.header.seq_num) + ", Data Size: " + std::to_string(pkt.header.length));
                outfile.write(pkt.payload, pkt.header.length);
                expected_seq_num += pkt.header.length;
            } else if (pkt.header.seq_num < expected_seq_num) {
                // Duplicate packet arrived (Previous ACK was likely lost in the network)
                log_msg("WARN", "Duplicate packet received (Seq: " + std::to_string(pkt.header.seq_num) + "). Re-sending ACK.");
            } else {
                // Out-of-order packet (Should not occur in pure Stop-and-Wait, but safely ignored)
                log_msg("WARN", "Out of order packet received. Dropping.");
                continue;
            }

            // Build and transmit ACK response packet
            Packet ack_pkt{};
            ack_pkt.header.ack_num = expected_seq_num; 
            ack_pkt.header.flags = FLAG_ACK;
            ack_pkt.header.length = 0;

            std::vector<uint8_t> temp_ack = serialize_packet(ack_pkt);
            ack_pkt.header.checksum = compute_checksum(temp_ack.data(), temp_ack.size());
            std::vector<uint8_t> final_ack = serialize_packet(ack_pkt);

            sendto(sockfd, final_ack.data(), final_ack.size(), 0, (struct sockaddr*)&src_addr, addr_len);
            log_msg("SEND", "Dispatched ACK: " + std::to_string(expected_seq_num));
        } 
        else if (pkt.header.flags & FLAG_FIN) {
            log_msg("RCVD", "FIN packet received. Replying with FIN-ACK.");
            
            Packet fin_ack{};
            fin_ack.header.ack_num = pkt.header.seq_num;
            fin_ack.header.flags = FLAG_ACK;
            fin_ack.header.length = 0;

            std::vector<uint8_t> temp_fa = serialize_packet(fin_ack);
            fin_ack.header.checksum = compute_checksum(temp_fa.data(), temp_fa.size());
            std::vector<uint8_t> final_fa = serialize_packet(fin_ack);

            sendto(sockfd, final_fa.data(), final_fa.size(), 0, (struct sockaddr*)&src_addr, addr_len);
            running = false;
        }
    }

    log_msg("SUCCESS", "File writing completed.");
    outfile.close();
    close(sockfd);
    return 0;
}