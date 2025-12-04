#include "NetworkManager.h"
#include "..\..\iocp_single\iocp_single\Protocol.h"
#include <iostream>
#include <cstring>

NetworkManager::NetworkManager()
    : m_inPacketSize(0)
    , m_savedPacketSize(0)
{
    memset(m_packetBuffer, 0, sizeof(m_packetBuffer));
}

NetworkManager::~NetworkManager()
{
    Disconnect();
}

bool NetworkManager::Connect(const std::string& ip, unsigned short port)
{
    sf::Socket::Status status = m_socket.connect(ip, port);
    
    if (status != sf::Socket::Done)
    {
        std::wcout << L"Server connection failed!" << std::endl;
        return false;
    }
    
    m_socket.setBlocking(false);
    std::wcout << L"Server connection success! (" << ip.c_str() << ":" << port << ")" << std::endl;
    return true;
}

void NetworkManager::Disconnect()
{
    m_socket.disconnect();
}

bool NetworkManager::Receive(char* buffer, size_t bufferSize, size_t& received)
{
    auto recvResult = m_socket.receive(buffer, bufferSize, received);
    
    if (recvResult == sf::Socket::Error)
    {
        std::wcout << L"Receive error!" << std::endl;
        return false;
    }
    
    if (recvResult == sf::Socket::Disconnected)
    {
        std::wcout << L"Server disconnected" << std::endl;
        return false;
    }
    
    return (recvResult == sf::Socket::Done || recvResult == sf::Socket::NotReady);
}

void NetworkManager::SendPacket(void* packet)
{
    char* p = reinterpret_cast<char*>(packet);
    size_t sent = 0;
    m_socket.send(p, p[0], sent);
}

void NetworkManager::SendLoginPacket(const std::string& id, int password)
{
    cs_packet_login packet;
    packet.size = sizeof(packet);
    packet.type = CS_PACKET_LOGIN;
    strcpy_s(packet.userID, id.c_str());
    packet.userPassWord = password;
    
    SendPacket(&packet);
    std::cout << "Login packet sent - ID: " << id << std::endl;
}

void NetworkManager::SendMovePacket(unsigned char direction)
{
    cs_packet_move packet;
    packet.type = CS_PACKET_MOVE;
    packet.size = sizeof(packet);
    packet.direction = direction;
    
    SendPacket(&packet);
}

void NetworkManager::SendAttackPacket()
{
    cs_packet_attack packet;
    packet.type = CS_PACKET_ATTACK;
    packet.size = sizeof(packet);
    
    SendPacket(&packet);
}

void NetworkManager::SendTeleportPacket()
{
    cs_packet_teleport packet;
    packet.type = CS_PACKET_TELEPORT;
    packet.size = sizeof(packet);
    
    SendPacket(&packet);
}
