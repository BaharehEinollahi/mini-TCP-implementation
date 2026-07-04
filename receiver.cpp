#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>
#include <map> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "packet.hpp"
#include "logger.hpp"

class UdpReceiver {
public:
    UdpReceiver(int port, const std::string& filename, double loss_prob = 0.0, int delay_ms = 0)
        : port(port), filename(filename), sockfd(-1), expected_seq_num(0), 
          loss_prob(loss_prob), delay_ms(delay_ms) {
        std::memset(&local_addr, 0, sizeof(local_addr));
    }

    ~UdpReceiver() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    bool init() {
        std::srand(std::time(nullptr));

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            log_msg("ERROR", "Failed to open socket");
            return false;
        }

        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;
        local_addr.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            log_msg("ERROR", "Bind failed");
            return false;
        }

        outfile.open(filename, std::ios::binary);
        if (!outfile.is_open()) {
            log_msg("ERROR", "Could not open output file: " + filename);
            return false;
        }

        return true;
    }

    void start_listening() {
        log_msg("INFO", "Receiver active. Ready for Stop-and-Wait frames...");

        uint8_t network_buffer[HEADER_SIZE + MAX_PAYLOAD_SIZE];
        sockaddr_in src_addr{};
        socklen_t addr_len = sizeof(src_addr);
        bool transfer_active = true;

        while (transfer_active) {
            ssize_t bytes_received = recvfrom(sockfd, network_buffer, sizeof(network_buffer), 0,
                                              (struct sockaddr*)&src_addr, &addr_len);
            if (bytes_received < static_cast<ssize_t>(HEADER_SIZE)) {
                continue;
            }

            if (loss_prob > 0.0) {
                double drop_roll = static_cast<double>(std::rand()) / RAND_MAX;
                if (drop_roll < loss_prob) {
                    log_msg("DROP", "Simulated Artificial Loss. Ignoring incoming packet completely.");
                    continue;
                }
            }

            if (!validate_checksum(network_buffer, bytes_received)) {
                log_msg("DROP", "Corrupted frame detected via checksum. Dropping.");
                continue;
            }

            Packet pkt = deserialize_packet(network_buffer, bytes_received);

            if (pkt.header.flags & FLAG_DATA) {
                handle_data_packet(pkt, src_addr, addr_len);
            } 
            else if (pkt.header.flags & FLAG_FIN) {
                handle_fin_packet(pkt, src_addr, addr_len);
                transfer_active = false;
            }
        }

        outfile.close();
    }

private:
    int port;
    std::string filename;
    int sockfd;
    sockaddr_in local_addr;
    std::ofstream outfile;
    uint32_t expected_seq_num;
    std::map<uint32_t, Packet> packet_buffer;
    
    double loss_prob;
    int delay_ms;

    bool validate_checksum(uint8_t* buffer, ssize_t length) {
        Packet pkt = deserialize_packet(buffer, length);
        uint16_t received_checksum = pkt.header.checksum;
        
        uint16_t zero_check = 0;
        std::memcpy(&buffer[10], &zero_check, 2);

        return compute_checksum(buffer, length) == received_checksum;
    }

    void send_ack(uint32_t ack_num, const sockaddr_in& src_addr, socklen_t addr_len) {
        if (delay_ms > 0) {
            usleep(delay_ms * 1000);
        }

        Packet ack_pkt{};
        ack_pkt.header.seq_num = 0;
        ack_pkt.header.ack_num = ack_num;
        ack_pkt.header.length = 0;
        ack_pkt.header.flags = FLAG_ACK;
        ack_pkt.header.checksum = 0;

        std::vector<uint8_t> ack_temp = serialize_packet(ack_pkt);
        ack_pkt.header.checksum = compute_checksum(ack_temp.data(), ack_temp.size());
        std::vector<uint8_t> ack_buf = serialize_packet(ack_pkt);

        sendto(sockfd, ack_buf.data(), ack_buf.size(), 0, (struct sockaddr*)&src_addr, addr_len);
    }

    void handle_data_packet(const Packet& pkt, const sockaddr_in& src_addr, socklen_t addr_len) {
        if (pkt.header.seq_num == expected_seq_num) {
            outfile.write(pkt.payload, pkt.header.length);
            expected_seq_num += pkt.header.length;
            log_msg("RCVD", "Accepted Seq: " + std::to_string(pkt.header.seq_num));

            while (packet_buffer.count(expected_seq_num)) {
                Packet next_pkt = packet_buffer[expected_seq_num];
                outfile.write(next_pkt.payload, next_pkt.header.length);
                packet_buffer.erase(expected_seq_num);
                expected_seq_num += next_pkt.header.length;
            }
        } else if (pkt.header.seq_num < expected_seq_num) {
            log_msg("WARN", "Received duplicate Seq: " + std::to_string(pkt.header.seq_num) + ". Dropping data payload.");
        } else {
            packet_buffer[pkt.header.seq_num] = pkt;
            log_msg("WARN", "Out-of-order Seq: " + std::to_string(pkt.header.seq_num) + " buffered.");
        }

        send_ack(expected_seq_num, src_addr, addr_len);
        log_msg("SEND", "Transmitted ACK for next expected Seq: " + std::to_string(expected_seq_num));
    }

    void handle_fin_packet(const Packet& pkt, const sockaddr_in& src_addr, socklen_t addr_len) {
        log_msg("RCVD", "FIN frame encountered.");
        send_ack(pkt.header.seq_num, src_addr, addr_len);
        log_msg("SEND", "Transmitted FIN-ACK frame. Closing down execution loop cleanly.");
    }
};

void read_cmd_args(int argc, char * argv[], int &port, std::string &filename, double &loss_prob, int &delay_ms ){
    port = std::stoi(argv[1]);
    filename = argv[2];

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--loss" && i + 1 < argc) {
            loss_prob = std::stod(argv[++i]);
        } else if (arg == "--delay" && i + 1 < argc) {
            delay_ms = std::stoi(argv[++i]);
        }
    }
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <output_file> [--loss <probability>] [--delay <ms>]" << std::endl;
        return 1;
    }

    int port;
    std::string filename;
    double loss_prob = 0.0;
    int delay_ms = 0;
    read_cmd_args(argc, argv, port, filename, loss_prob, delay_ms);


    UdpReceiver receiver(port, filename, loss_prob, delay_ms);
    if (!receiver.init()) {
        return 1;
    }

    receiver.start_listening();
    return 0;
}