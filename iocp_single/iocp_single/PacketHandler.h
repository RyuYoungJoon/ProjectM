#pragma once
#include "pch.h"

class Client;
class Sender;
class NPCManager;
class TimerManager;

class PacketHandler {
public:
    PacketHandler(
        std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& clients,
        Sender& sender,
        NPCManager& npcManager,
        TimerManager& timerManager,
        MAP (&gameMap)[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH]
    );

    void ProcessPacket(int clientId, char* packet);

private:
    void HandleLogin(int clientId, char* packet);
    void HandleMove(int clientId, char* packet);
    void HandleAttack(int clientId, char* packet);
    void HandleChat(int clientId, char* packet);
    void HandleTeleport(int clientId, char* packet);
    void HandleLogout(int clientId, char* packet);

    // 게임 로직 헬퍼
    void DoMove(int clientId, char direction, int moveTime);
    void AccessGame(int userId, const char* name);
    bool IsAttackable(int attackerId, int targetId) const;
    void AddExp(int clientId, short value);
    void AddLevel(int clientId, short value);
    void AddHp(int clientId, short value);

    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& m_clients;
    Sender& m_sender;
    NPCManager& m_npcManager;
    TimerManager& m_timerManager;
    MAP (&m_gameMap)[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH];
    int m_acceptedPlayerCount;
};
