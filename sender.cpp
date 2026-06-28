#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "packet.hpp"
#include "logger.hpp"

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

    log_msg("INFO", "Starting file transmission: " + filename);

    uint32_t seq_num = 0;
    char file_buffer[MAX_PAYLOAD_SIZE];

    while (infile.read(file_buffer, MAX_PAYLOAD_SIZE) || infile.gcount() > 0) {
        size_t bytes_read = infile.gcount();

        Packet pkt{};
        pkt.header.seq_num = seq_num;
        pkt.header.ack_num = 0;
        pkt.header.length = static_cast<uint16_t>(bytes_read);
        pkt.header.flags = FLAG_DATA;
        pkt.header.checksum = 0; 
        std::memcpy(pkt.payload, file_buffer, bytes_read);

        // Serialize layout temporarily to compute real checksum over complete frame
        std::vector<uint8_t> temp_buf = serialize_packet(pkt);
        pkt.header.checksum = compute_checksum(temp_buf.data(), temp_buf.size());

        // Re-serialize with valid checksum calculated
        std::vector<uint8_t> final_buf = serialize_packet(pkt);

        sendto(sockfd, final_buf.data(), final_buf.size(), 0, 
               (struct sockaddr*)&rec_addr, sizeof(rec_addr));
        
        log_msg("SEND", "Sent DATA packet. Seq: " + std::to_string(seq_num) + ", Bytes: " + std::to_string(bytes_read));
        
        seq_num += bytes_read;
        usleep(5000); // Small pacing delay for phase zero stability
    }

    // Send FIN packet to signal completion
    Packet fin_pkt{};
    fin_pkt.header.seq_num = seq_num;
    fin_pkt.header.ack_num = 0;
    fin_pkt.header.length = 0;
    fin_pkt.header.flags = FLAG_FIN;
    fin_pkt.header.checksum = 0;

    std::vector<uint8_t> fin_buf = serialize_packet(fin_pkt);
    fin_pkt.header.checksum = compute_checksum(fin_buf.data(), fin_buf.size());
    fin_buf = serialize_packet(fin_pkt);

    sendto(sockfd, fin_buf.data(), fin_buf.size(), 0, (struct sockaddr*)&rec_addr, sizeof(rec_addr));
    log_msg("SEND", "Sent FIN packet. Signaling completion.");

    infile.close();
    close(sockfd);
    return 0;
}