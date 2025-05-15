#include "../include/packet.h"
#include <sstream>
#include <cstdio>

namespace transport {

uint32_t calculate_checksum(const char* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i)
        sum += static_cast<unsigned char>(data[i]);
    return sum;
}

std::unordered_map<uint32_t, int> parse_loss_pattern(const std::string& spec) {
    std::unordered_map<uint32_t, int> map;
    std::istringstream ss(spec);
    std::string part;
    while (std::getline(ss, part, ',')) {
        uint32_t seq;
        int count;
        if (sscanf(part.c_str(), "%u:%d", &seq, &count) == 2)
            map[seq] = count;
    }
    return map;
}

}