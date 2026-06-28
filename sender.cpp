#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include "packet.hpp"
#include "logger.hpp"
#include "timer.hpp"

constexpr int TIMEOUT_MS = 200; // 200 milliseconds fixed timeout

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <receiver_port> <input_file>" << std::endl;
        return 1;
    }

    std::string rec_ip = argv[1];
    int rec_port = std::stoi(argv[2]);
    std::string filename = argv[3];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_msg("ERROR", "Failed to create socket");
        return 1;
    }

    // Set socket receive timeout for ACK handling
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT_MS * 1000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        log_msg("ERROR", "Failed to set socket timeout options");
        close(sockfd);
        return 1;
    }

    sockaddr_in rec_addr{};
    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(rec_port);
    inet_pton(AF_INET, rec_ip.c_str(), &rec_addr.sin_addr);

    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        log_msg("ERROR", "Could not open input file: " + filename);
        close(sockfd);
        return 1;
    }

    log_msg("INFO", "Starting Stop-and-Wait transmission: " + filename);
    
    Timer total_transmission_timer;
    total_transmission_timer.start();

    uint32_t seq_num = 0;
    char file_buffer[MAX_PAYLOAD_SIZE];

    while (infile.read(file_buffer, MAX_PAYLOAD_SIZE) || infile.gcount() > 0) {
        size_t bytes_read = infile.gcount();

        // 1. Construct the data packet
        Packet pkt{};
        pkt.header.seq_num = seq_num;
        pkt.header.ack_num = 0;
        pkt.header.length = static_cast<uint16_t>(bytes_read);
        pkt.header.flags = FLAG_DATA;
        std::memcpy(pkt.payload, file_buffer, bytes_read);

        // Compute checksum
        std::vector<uint8_t> temp_buf = serialize_packet(pkt);
        pkt.header.checksum = compute_checksum(temp_buf.data(), temp_buf.size());
        std::vector<uint8_t> final_pkt_buf = serialize_packet(pkt);

        bool ack_received = false;
        int retransmissions = 0;

        // 2. Stop-and-Wait Loop
        while (!ack_received) {
            log_msg("SEND", "Sending Packet. Seq: " + std::to_string(seq_num) + 
                    ", Attempt: " + std::to_string(retransmissions + 1));
            
            sendto(sockfd, final_pkt_buf.data(), final_pkt_buf.size(), 0, 
                   (struct sockaddr*)&rec_addr, sizeof(rec_addr));

            // Wait for matching ACK
            uint8_t ack_buffer[HEADER_SIZE + MAX_PAYLOAD_SIZE];
            sockaddr_in src_addr{};
            socklen_t addr_len = sizeof(src_addr);

            ssize_t bytes_recvd = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0,
                                           (struct sockaddr*)&src_addr, &addr_len);

            if (bytes_recvd < 0) {
                // Socket timeout triggered (EAGAIN or EWOULDBLOCK)
                log_msg("TIMEOUT", "ACK timeout for Seq: " + std::to_string(seq_num) + ". Retransmitting...");
                retransmissions++;
                continue;
            }

            // Deserialize and validate ACK packet
            Packet ack_pkt = deserialize_packet(ack_buffer, bytes_recvd);
            
            // Validate Checksum of ACK Frame
            uint16_t received_chk = ack_pkt.header.checksum;
            uint16_t zero_chk = 0;
            std::memcpy(&ack_buffer[10], &zero_chk, 2);
            uint16_t computed_chk = compute_checksum(ack_buffer, bytes_recvd);

            if (received_chk != computed_chk) {
                log_msg("DROP", "Corrupted ACK packet received. Ignoring.");
                continue;
            }

            // Verify if ACK points to our expected cumulative sequence byte status
            if ((ack_pkt.header.flags & FLAG_ACK) && (ack_pkt.header.ack_num == seq_num + bytes_read)) {
                log_msg("ACK_RCVD", "Received expected ACK: " + std::to_string(ack_pkt.header.ack_num));
                ack_received = true;
            } else {
                log_msg("WARN", "Received out-of-order or duplicate ACK: " + std::to_string(ack_pkt.header.ack_num));
            }
        }

        seq_num += bytes_read;
    }

    // 3. Reliable Teardown (FIN Handshake Sequence)
    bool fin_ack_received = false;
    Packet fin_pkt{};
    fin_pkt.header.seq_num = seq_num;
    fin_pkt.header.flags = FLAG_FIN;
    std::vector<uint8_t> temp_fin = serialize_packet(fin_pkt);
    fin_pkt.header.checksum = compute_checksum(temp_fin.data(), temp_fin.size());
    std::vector<uint8_t> final_fin = serialize_packet(fin_pkt);

    while (!fin_ack_received) {
        log_msg("SEND", "Sending FIN packet.");
        sendto(sockfd, final_fin.data(), final_fin.size(), 0, (struct sockaddr*)&rec_addr, sizeof(rec_addr));

        uint8_t ack_buffer[HEADER_SIZE];
        ssize_t bytes_recvd = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, nullptr, nullptr);
        if (bytes_recvd > 0) {
            Packet ack_pkt = deserialize_packet(ack_buffer, bytes_recvd);
            if ((ack_pkt.header.flags & FLAG_ACK) && (ack_pkt.header.ack_num == seq_num)) {
                log_msg("ACK_RCVD", "FIN-ACK received smoothly.");
                fin_ack_received = true;
            }
        }
    }

    total_transmission_timer.stop();
    log_msg("SUCCESS", "File transfer finished successfully!");
    log_msg("METRIC", "Total File Transfer Time: " + std::to_string(total_transmission_timer.elapsed_ms()) + " ms");

    infile.close();
    close(sockfd);
    return 0;
}