#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../include/packet.h"

constexpr int BUFFER_SIZE = 1024;
constexpr int HEADER_SIZE = sizeof(transport::PacketHeader);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <loss_pattern ex. 1:1,3:2>\n";
        return 1;
    }

    auto loss_map = transport::parse_loss_pattern(argv[1]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = 0;

    bind(sock, (sockaddr*)&server_addr, sizeof(server_addr));

    socklen_t addrlen = sizeof(server_addr);
    getsockname(sock, (sockaddr*)&server_addr, &addrlen);
    std::cout << "Server listening on port: " << ntohs(server_addr.sin_port) << "\n";

    std::unordered_map<uint32_t, std::vector<char>> received_data;
    sockaddr_in client_addr{};

    while (true) {
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(client_addr);
        ssize_t n = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                             (sockaddr*)&client_addr, &len);
        if (n < HEADER_SIZE) continue;

        transport::PacketHeader header;
        memcpy(&header, buffer, HEADER_SIZE);

        auto data_ptr = buffer + HEADER_SIZE;

        if (transport::calculate_checksum(data_ptr, header.length) != header.checksum) {
            std::cerr << "Packet " << header.seq << " corrupted\n";
            continue;
        }

        if (loss_map[header.seq] > 0) {
            std::cout << "Simulating drop of packet " << header.seq << "\n";
            loss_map[header.seq]--;
            continue;
        }

        received_data[header.seq] = std::vector<char>(data_ptr, data_ptr + header.length);

        sendto(sock, &header.seq, sizeof(header.seq), 0,
               (sockaddr*)&client_addr, len);

        std::cout << "Received and ACKed packet: " << header.seq << "\n";
    }
}