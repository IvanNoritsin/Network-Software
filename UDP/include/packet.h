#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace transport {

struct PacketHeader {
    uint32_t seq;
    uint16_t length;
    uint32_t checksum;
};

uint32_t calculate_checksum(const char* data, size_t len);
std::unordered_map<uint32_t, int> parse_loss_pattern(const std::string& spec);

}

#endif