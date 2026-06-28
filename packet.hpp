#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

constexpr uint16_t MAX_PAYLOAD_SIZE = 1024;
constexpr size_t HEADER_SIZE = 13; // 4 + 4 + 2 + 2 + 1 bytes

// Flags definitions
constexpr uint8_t FLAG_DATA = 0x01;
constexpr uint8_t FLAG_ACK  = 0x02;
constexpr uint8_t FLAG_FIN  = 0x04;

struct PacketHeader {  
    uint32_t seq_num;  
    uint32_t ack_num;  
    uint16_t length;  
    uint16_t checksum;  
    uint8_t flags;  
};

struct Packet {
    PacketHeader header;
    char payload[MAX_PAYLOAD_SIZE];
};

// Computes standard 16-bit Internet Checksum
inline uint16_t compute_checksum(const uint8_t* data, size_t length) {
    uint32_t sum = 0;
    for (size_t i = 0; i < length - 1; i += 2) {
        sum += (data[i] << 8) | data[i + 1];
    }
    if (length & 1) {
        sum += (data[length - 1] << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~static_cast<uint16_t>(sum);
}

// Serializes a Packet struct into a raw byte vector (Network Byte Order)
inline std::vector<uint8_t> serialize_packet(const Packet& packet) {
    std::vector<uint8_t> buffer(HEADER_SIZE + packet.header.length);

    uint32_t net_seq = htonl(packet.header.seq_num);
    uint32_t net_ack = htonl(packet.header.ack_num);
    uint16_t net_len = htons(packet.header.length);
    uint16_t net_chk = htons(packet.header.checksum);

    std::memcpy(&buffer[0], &net_seq, 4);
    std::memcpy(&buffer[4], &net_ack, 4);
    std::memcpy(&buffer[8], &net_len, 2);
    std::memcpy(&buffer[10], &net_chk, 2);
    buffer[12] = packet.header.flags;

    if (packet.header.length > 0) {
        std::memcpy(&buffer[HEADER_SIZE], packet.payload, packet.header.length);
    }

    return buffer;
}

// Deserializes a raw byte stream into a Packet struct (Host Byte Order)
inline Packet deserialize_packet(const uint8_t* buffer, size_t total_length) {
    Packet packet;
    
    uint32_t net_seq, net_ack;
    uint16_t net_len, net_chk;

    std::memcpy(&net_seq, &buffer[0], 4);
    std::memcpy(&net_ack, &buffer[4], 4);
    std::memcpy(&net_len, &buffer[8], 2);
    std::memcpy(&net_chk, &buffer[10], 2);
    packet.header.flags = buffer[12];

    packet.header.seq_num = ntohl(net_seq);
    packet.header.ack_num = ntohl(net_ack);
    packet.header.length = ntohs(net_len);
    packet.header.checksum = ntohs(net_chk);

    if (packet.header.length > 0 && (HEADER_SIZE + packet.header.length <= total_length)) {
        std::memcpy(packet.payload, &buffer[HEADER_SIZE], packet.header.length);
    }

    return packet;
}

#endif