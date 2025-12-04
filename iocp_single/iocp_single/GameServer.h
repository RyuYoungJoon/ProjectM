#pragma once
#include "pch.h"

// Forward declarations
class Client;
class Sender;
class PacketHandler;
class NPCManager;
class TimerManager;
struct MAP;

class GameServer {
public:
    GameServer();
    ~GameServer();

    bool Initialize();
    void Run();
    void Shutdown();

    // Client Management
    int GetNewClientId();
    void DisconnectClient(int clientId);
    bool IsNear(int id1, int id2) const;
    bool IsNPC(int id) const;
    bool IsPlayer(int id) const;

    // Game Logic
    void PlayerHeal(int clientId);

    // Getters - implemented in cpp
    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& GetClients();
    HANDLE GetIOCP() const;
    MAP (&GetGameMap())[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH];

private:
    void InitializeMap();
    void WorkerThread();
    void HandleRecv(int clientId, Exp_Over* expOver, DWORD numBytes);
    void HandleSend(int clientId, Exp_Over* expOver, DWORD numBytes);
    void HandleAccept(Exp_Over* expOver);
    void HandleNPCMove(int clientId, Exp_Over* expOver);
    void HandleAttack(int clientId, Exp_Over* expOver);
    void HandleRevive(int clientId, Exp_Over* expOver);
    void HandleHeal(int clientId, Exp_Over* expOver);

    // Network
    SOCKET m_listenSocket;
    HANDLE m_hIOCP;

    // Game Data
    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC> m_clients;
    MAP m_gameMap[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH];
    std::atomic<int> m_acceptedPlayerCount;

    // Managers
    std::unique_ptr<Sender> m_sender;
    std::unique_ptr<PacketHandler> m_packetHandler;
    std::unique_ptr<NPCManager> m_npcManager;
    std::unique_ptr<TimerManager> m_timerManager;

    // Worker Threads
    std::vector<std::unique_ptr<std::thread>> m_workerThreads;
    std::atomic<bool> m_running;
};
