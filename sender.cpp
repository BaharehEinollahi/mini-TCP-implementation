#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include "packet.hpp"
#include "logger.hpp"
#include "timer.hpp"

enum SenderAlgorithm{
    STOP_AND_WAIT,
    SLIDING_WINDOW
};

class UdpSender {
public:
    UdpSender(const std::string& ip, int port) 
        : rec_ip(ip), rec_port(port), sockfd(-1), total_packets_sent(0), total_retransmissions(0) {
        std::memset(&rec_addr, 0, sizeof(rec_addr));
    }

    ~UdpSender() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    bool init() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            log_msg("ERROR", "Failed to create socket");
            return false;
        }

        struct timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 20000; 
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        rec_addr.sin_family = AF_INET;
        rec_addr.sin_port = htons(rec_port);
        if (inet_pton(AF_INET, rec_ip.c_str(), &rec_addr.sin_addr) <= 0) {
            log_msg("ERROR", "Invalid receiver IP address");
            return false;
        }

        return true;
    }

    bool transfer_file(SenderAlgorithm alg, const std::string& filename) {
        std::ifstream infile(filename, std::ios::binary);
        if (!infile.is_open()) {
            log_msg("ERROR", "Could not open input file: " + filename);
            return false;
        }

        total_transfer_timer.start();
        uint32_t seq_num = 0;
        bool success = false;

        if (alg == STOP_AND_WAIT) {
            log_msg("INFO", "Starting Stop-and-Wait transmission...");
            success = run_stop_and_wait(infile, seq_num);
        } else if (alg == SLIDING_WINDOW) {
            log_msg("INFO", "Starting Sliding Window transmission...");
            success = run_sliding_window(infile, seq_num);
        }

        if (success) {
            std::vector<uint8_t> fin_buf = build_packet(seq_num, FLAG_FIN, nullptr, 0);
            send_and_wait(fin_buf, seq_num, seq_num, "FIN");

            total_transfer_timer.stop();
            infile.close();
            print_metrics();
            return true;
        }

        infile.close();
        return false;
    }

private:
    std::string rec_ip;
    int rec_port;
    int sockfd;
    sockaddr_in rec_addr;

    uint32_t total_packets_sent;
    uint32_t total_retransmissions;
    Timer total_transfer_timer;
    Timer packet_timer;

    bool run_stop_and_wait(std::ifstream& infile, uint32_t& seq_num) {
        char file_buffer[MAX_PAYLOAD_SIZE];

        while (infile.read(file_buffer, MAX_PAYLOAD_SIZE) || infile.gcount() > 0) {
            size_t bytes_read = infile.gcount();
            
            std::vector<uint8_t> final_buf = build_packet(seq_num, FLAG_DATA, file_buffer, bytes_read);
            uint32_t expected_ack = seq_num + bytes_read;
            
            send_and_wait(final_buf, expected_ack, seq_num, "DATA");
            seq_num += bytes_read;
        }
        return true;
    }

    bool run_sliding_window(std::ifstream& infile, uint32_t& seq_num) {
        const size_t WINDOW_SIZE = 5; 
        
        struct WindowPacket {
            uint32_t seq_num;
            uint32_t expected_ack;
            std::vector<uint8_t> data;
        };
        std::vector<WindowPacket> window;

        bool eof_reached = false;
        char file_buffer[MAX_PAYLOAD_SIZE];
        Timer window_timer;
        bool timer_running = false;

        while (!eof_reached || !window.empty()) {
            while (window.size() < WINDOW_SIZE && !eof_reached) {
                if (infile.read(file_buffer, MAX_PAYLOAD_SIZE) || infile.gcount() > 0) {
                    size_t bytes_read = infile.gcount();
                    std::vector<uint8_t> pkt_data = build_packet(seq_num, FLAG_DATA, file_buffer, bytes_read);
                    uint32_t expected_ack = seq_num + bytes_read;
                    
                    window.push_back({seq_num, expected_ack, pkt_data});
                    
                    sendto(sockfd, pkt_data.data(), pkt_data.size(), 0, (struct sockaddr*)&rec_addr, sizeof(rec_addr));
                    total_packets_sent++;
                    log_msg("SEND", "Sent DATA Packet (Seq: " + std::to_string(seq_num) + 
                                    ", Bytes: " + std::to_string(pkt_data.size()) + ")");
                    
                    if (!timer_running) {
                        window_timer.start();
                        timer_running = true;
                    }
                    seq_num += bytes_read;
                } else {
                    eof_reached = true;
                }
            }

            uint8_t ack_buffer[HEADER_SIZE + MAX_PAYLOAD_SIZE];
            sockaddr_in src_addr{};
            socklen_t addr_len = sizeof(src_addr);

            ssize_t res = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr*)&src_addr, &addr_len);
            
            if (res >= static_cast<ssize_t>(HEADER_SIZE)) {
                Packet ack_pkt = deserialize_packet(ack_buffer, res);
                uint16_t received_chk = ack_pkt.header.checksum;
                
                uint16_t zero_check = 0;
                std::memcpy(&ack_buffer[10], &zero_check, 2); 

                if (compute_checksum(ack_buffer, res) == received_chk && (ack_pkt.header.flags & FLAG_ACK)) {
                    uint32_t ack_num = ack_pkt.header.ack_num;
                    bool advanced_window = false;

                    auto it = window.begin();
                    while (it != window.end()) {
                        if (it->expected_ack <= ack_num) {
                            it = window.erase(it);
                            advanced_window = true;
                        } else {
                            break;
                        }
                    }

                    if (advanced_window) {
                        log_msg("ACK ", "Window advanced to ACK: " + std::to_string(ack_num));
                        if (window.empty()) {
                            timer_running = false;
                        } else {
                            window_timer.start(); 
                        }
                    }
                }
            }

            if (timer_running && window_timer.elapsed_ms() >= 1000.0) {
                log_msg("TIMEOUT", "RTO expired. Retransmitting entire window...");
                for (const auto& pkt : window) {
                    sendto(sockfd, pkt.data.data(), pkt.data.size(), 0, (struct sockaddr*)&rec_addr, sizeof(rec_addr));
                    total_packets_sent++;
                    total_retransmissions++;
                    log_msg("SEND", "Retransmitted DATA Packet (Seq: " + std::to_string(pkt.seq_num) + ")");
                }
                window_timer.start(); 
            }
        }
        return true;
    }

    std::vector<uint8_t> build_packet(uint32_t seq_num, uint8_t flags, const char* payload, size_t length) {
        Packet pkt{};
        pkt.header.seq_num = seq_num;
        pkt.header.ack_num = 0;
        pkt.header.length = static_cast<uint16_t>(length);
        pkt.header.flags = flags;
        pkt.header.checksum = 0;
        
        if (payload && length > 0) {
            std::memcpy(pkt.payload, payload, length);
        }

        std::vector<uint8_t> temp_buf = serialize_packet(pkt);
        pkt.header.checksum = compute_checksum(temp_buf.data(), temp_buf.size());
        return serialize_packet(pkt);
    }

    
    bool receive_and_validate_ack(uint32_t expected_ack, uint8_t expected_flag) {
        uint8_t ack_buffer[HEADER_SIZE + MAX_PAYLOAD_SIZE];
        sockaddr_in src_addr{};
        socklen_t addr_len = sizeof(src_addr);

        ssize_t res = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr*)&src_addr, &addr_len);
        if (res < static_cast<ssize_t>(HEADER_SIZE)) {
            return false;
        }

        Packet ack_pkt = deserialize_packet(ack_buffer, res);
        uint16_t received_chk = ack_pkt.header.checksum;
        
        
        uint16_t zero_check = 0;
        std::memcpy(&ack_buffer[10], &zero_check, 2);

        if (compute_checksum(ack_buffer, res) == received_chk) {
            if ((ack_pkt.header.flags & expected_flag) && (ack_pkt.header.ack_num == expected_ack)) {
                return true;
            }
        }
        return false;
    }

    
    void send_and_wait(const std::vector<uint8_t>& packet_data,
                    uint32_t expected_ack,
                    uint32_t current_seq,
                    const std::string& pkt_type) {
        bool ack_received = false;

        
        sendto(sockfd, packet_data.data(), packet_data.size(), 0,
            (struct sockaddr*)&rec_addr, sizeof(rec_addr));

        total_packets_sent++;
        log_msg("SEND", "Sent " + pkt_type +
                        " Packet (Seq: " + std::to_string(current_seq) +
                        ", Bytes: " + std::to_string(packet_data.size()) + ")");
        packet_timer.start();

        while (!ack_received) {
            uint8_t target_flag = FLAG_ACK;

            if (receive_and_validate_ack(expected_ack, target_flag)) {
                ack_received = true;
                packet_timer.stop();

                log_msg("ACK ", "Received valid ACK for " + pkt_type +
                            " (Seq: " + std::to_string(current_seq) + ")");
                break;
            }

            if (packet_timer.elapsed_ms() >= 1000.0) {
                log_msg("TIMEOUT",
                        "RTO expired for " + pkt_type +
                        " Packet (Seq: " + std::to_string(current_seq) +
                        "). Retransmitting...");

                sendto(sockfd, packet_data.data(), packet_data.size(), 0,
                    (struct sockaddr*)&rec_addr, sizeof(rec_addr));

                total_retransmissions++;
                total_packets_sent++;

                log_msg("SEND", "Retransmitted " + pkt_type +
                                " Packet (Seq: " + std::to_string(current_seq) +
                                ", Bytes: " + std::to_string(packet_data.size()) + ")");

                packet_timer.start();
            }
        }
    }

    void print_metrics() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "        PHASE 1 TRANSFER METRICS        " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total Packets Sent (incl. retrans): " << total_packets_sent << std::endl;
        std::cout << "Number of Retransmissions         : " << total_retransmissions << std::endl;
        std::cout << "Total File Transfer Time          : " << total_transfer_timer.elapsed_ms() << " ms" << std::endl;
        std::cout << "========================================" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <receiver_port> <input_file>" << std::endl;
        return 1;
    }

    std::string rec_ip = argv[1];
    int rec_port = std::stoi(argv[2]);
    std::string filename = argv[3];

    UdpSender sender(rec_ip, rec_port);
    if (!sender.init()) {
        return 1;
    }

    if (!sender.transfer_file(SLIDING_WINDOW, filename)) {
        return 1;
    }

    return 0;
}