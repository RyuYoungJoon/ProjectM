#pragma once
#include <SFML/Network.hpp>
#include <string>
#include "Config.h"

// Protocol.h will define SERVER_PORT

class NetworkManager {
private:
    sf::TcpSocket m_socket;
    char m_packetBuffer[200];  // BUF_SIZE
    size_t m_inPacketSize;
    size_t m_savedPacketSize;
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool Connect(const std::string& ip, unsigned short port);
    void Disconnect();
    
    bool Receive(char* buffer, size_t bufferSize, size_t& received);
    void SendPacket(void* packet);
    
    // Packet send functions
    void SendLoginPacket(const std::string& id, int password);
    void SendMovePacket(unsigned char direction);
    void SendAttackPacket();
    void SendTeleportPacket();
    
    sf::TcpSocket& GetSocket() { return m_socket; }
};
