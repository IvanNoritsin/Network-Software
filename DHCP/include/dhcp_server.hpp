#ifndef DHCP_SERVER
#define DHCP_SERVER

#include <string>
#include <map>
#include "dhcp_packet.hpp"

class DHCPServer {

public:
    DHCPServer();
    ~DHCPServer();
    void run();

private:
    int sock;
    std::string poolStart = "192.168.1.100";
    std::string poolEnd = "192.168.1.200";
    std::map<std::string, std::string> leases;

    void handleDiscover(const DHCPPacket& packet);
    void handleRequest(const DHCPPacket& packet);
    std::string getNewIP();
    void sendOffer(const DHCPPacket& req, const std::string& ip, const std::string& mac);
    void sendAck(const DHCPPacket& req, const std::string& ip, const std::string& mac);
};

#endif