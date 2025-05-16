#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../include/packet.h"

constexpr int CHUNK_SIZE = 900;
constexpr int HEADER_SIZE = sizeof(transport::PacketHeader);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port> <file>\n";
        return 1;
    }

    const char* ip = argv[1];
    int port = std::stoi(argv[2]);
    const char* filename = argv[3];

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "File not found: " << filename << "\n";
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    uint32_t seq = 0;
    while (!file.eof()) {
        std::vector<char> chunk(CHUNK_SIZE);
        file.read(chunk.data(), CHUNK_SIZE);
        std::streamsize len = file.gcount();

        transport::PacketHeader header{ seq, static_cast<uint16_t>(len),
            transport::calculate_checksum(chunk.data(), len) };

        std::vector<char> packet(HEADER_SIZE + len);
        memcpy(packet.data(), &header, HEADER_SIZE);
        memcpy(packet.data() + HEADER_SIZE, chunk.data(), len);

        while (true) {
            sendto(sock, packet.data(), packet.size(), 0,
                   (sockaddr*)&server_addr, sizeof(server_addr));

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock, &fds);

            timeval tv{1, 0};
            if (select(sock + 1, &fds, nullptr, nullptr, &tv) > 0) {
                uint32_t ack;
                socklen_t len = sizeof(server_addr);
                recvfrom(sock, &ack, sizeof(ack), 0, (sockaddr*)&server_addr, &len);
                if (ack == seq) break;
            } else {
                std::cout << "Packet:\t" << seq << "\tTimeout. Resending" << "\n";
            }
        }

        std::cout << "Packet:\t" << seq << "\tSent and ACKed" << "\n";
        seq++;
    }

    close(sock);
    return 0;
}