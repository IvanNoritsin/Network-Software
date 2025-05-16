#include "../include/dhcp_server.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

DHCPServer::DHCPServer() {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("Socket error");
        exit(1);
    }

    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(67);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind error");
        close(sock);
        exit(1);
    }
}

DHCPServer::~DHCPServer() {
    close(sock);
}

void DHCPServer::run() {
    std::cout << "DHCP Server started...\n";
    while (true) {
        DHCPPacket packet{};
        struct sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        ssize_t bytes = recvfrom(sock, &packet, sizeof(packet), 0,
                                 (struct sockaddr*)&clientAddr, &clientLen);

        if (bytes < 0) {
            perror("Recvfrom error");
            continue;
        }

        // Проверка магического cookie
        if (packet.magic_cookie != htonl(0x63825363)) {
            std::cerr << "Invalid DHCP magic cookie\n";
            continue;
        }

        // Определение типа сообщения
        uint8_t messageType = 0;
        size_t i = 0;
        while (i < sizeof(packet.options)) {
            uint8_t opt_code = packet.options[i];
            if (opt_code == 53) { 
                if (i + 2 < sizeof(packet.options)) {
                    messageType = packet.options[i + 2];
                    break;
                }
            }
            if (opt_code == 255) break;
            if (i + 1 >= sizeof(packet.options)) break;
            uint8_t opt_len = packet.options[i + 1];
            i += 2 + opt_len;
        }

        switch (messageType) {
            case 1: handleDiscover(packet); break;
            case 3: handleRequest(packet); break;
            default: break;
        }
    }
}

std::string DHCPServer::getNewIP() {
    static uint32_t current = ntohl(inet_addr(poolStart.c_str()));
    uint32_t end = ntohl(inet_addr(poolEnd.c_str()));

    while (current <= end) {
        in_addr addr{};
        addr.s_addr = htonl(current);
        std::string ip = inet_ntoa(addr);

        bool taken = false;
        for (const auto& pair : leases) {
            if (pair.second == ip) {
                taken = true;
                break;
            }
        }

        if (!taken) {
            current++;
            return ip;
        }

        current++;
    }

    return "";
}

void DHCPServer::handleDiscover(const DHCPPacket& packet) {
    std::string mac;
    for (int i = 0; i < 6; i++) {
        char buf[3];
        sprintf(buf, "%02x", packet.chaddr[i]);
        mac += buf;
        if (i < 5) mac += ":";
    }

    std::cout << "Got\tDHCPDISCOVER\tfrom\t" << mac << std::endl;

    auto it = leases.find(mac);
    std::string ip = (it != leases.end()) ? it->second : getNewIP();
    if (!ip.empty()) {
        leases[mac] = ip;
        sendOffer(packet, ip, mac);
        std::cout << "Sent\tOFFER\t\tto\t" << mac << "\tOffered IP:\t" << ip << std::endl;
    }
}

void DHCPServer::handleRequest(const DHCPPacket& packet) {
    std::string mac;
    for (int i = 0; i < 6; i++) {
        char buf[3];
        sprintf(buf, "%02x", packet.chaddr[i]);
        mac += buf;
        if (i < 5) mac += ":";
    }
    
    std::cout << "Got\tDHCPREQUEST\tfrom\t" << mac << std::endl;


    if (leases.count(mac)) {
        sendAck(packet, leases[mac], mac);
        std::cout << "Sent\tACK\t\tto\t" << mac << "\tAck IP:\t\t" << leases[mac] << std::endl;
    }
}

void DHCPServer::sendOffer(const DHCPPacket& req, const std::string& ip, const std::string& mac) {
    DHCPPacket resp{};
    memset(&resp, 0, sizeof(resp));

    resp.op = 2;
    resp.htype = 1;
    resp.hlen = 6;
    resp.xid = req.xid;
    resp.yiaddr = inet_addr(ip.c_str());
    resp.siaddr = inet_addr("192.168.1.1"); // IP сервера
    resp.magic_cookie = htonl(0x63825363);
    memcpy(resp.chaddr, req.chaddr, 16);

    uint8_t options[] = {
        53, 1, 2,                       // DHCP Message Type: Offer
        54, 4, 192, 168, 1, 1,          // DHCP Server Identifier
        51, 4, 0x00, 0x01, 0x51, 0x80,  // Lease Time (86400)
        1, 4, 255, 255, 255, 0,         // Subnet Mask
        3, 4, 192, 168, 1, 1,           // Router (Gateway)
        6, 4, 8, 8, 8, 8,               // DNS Server
        255                             // End
    };
    memcpy(resp.options, options, sizeof(options));

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(68);
    dest.sin_addr.s_addr = inet_addr("192.168.1.255");

    size_t packet_size = offsetof(DHCPPacket, options) + sizeof(options);
    
    if (sendto(sock, &resp, packet_size, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        perror("sendto OFFER failed");
    }
}

void DHCPServer::sendAck(const DHCPPacket& req, const std::string& ip, const std::string& mac) {
    DHCPPacket resp{};
    memset(&resp, 0, sizeof(resp));

    resp.op = 2;
    resp.htype = 1;
    resp.hlen = 6;
    resp.xid = req.xid;
    resp.yiaddr = inet_addr(ip.c_str());
    resp.siaddr = inet_addr("192.168.1.1"); // IP сервера
    resp.magic_cookie = htonl(0x63825363);
    memcpy(resp.chaddr, req.chaddr, 16);

    uint8_t options[] = {
        53, 1, 5,                       // DHCP Message Type: ACK
        54, 4, 192, 168, 1, 1,          // DHCP Server Identifier
        51, 4, 0x00, 0x01, 0x51, 0x80,  // Lease Time
        1, 4, 255, 255, 255, 0,         // Subnet Mask
        3, 4, 192, 168, 1, 1,           // Router
        6, 4, 8, 8, 8, 8,               // DNS
        255                             // End
    };
    memcpy(resp.options, options, sizeof(options));

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(68);
    dest.sin_addr.s_addr = inet_addr("192.168.1.255");

    sendto(sock, &resp, sizeof(DHCPPacket) - sizeof(resp.options) + sizeof(options), 0,
           (struct sockaddr*)&dest, sizeof(dest));
}
