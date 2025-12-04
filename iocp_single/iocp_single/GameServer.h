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

    // Client 관리
    int GetNewClientId();
    void DisconnectClient(int clientId);
    bool IsNear(int id1, int id2) const;
    bool IsNPC(int id) const;
    bool IsPlayer(int id) const;

    // 게임 로직
    void PlayerHeal(int clientId);

    // Getter - cpp에서 구현
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

    // 네트워크
    SOCKET m_listenSocket;
    HANDLE m_hIOCP;

    // 게임 데이터
    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC> m_clients;
    MAP m_gameMap[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH];
    std::atomic<int> m_acceptedPlayerCount;

    // 매니저들
    std::unique_ptr<Sender> m_sender;
    std::unique_ptr<PacketHandler> m_packetHandler;
    std::unique_ptr<NPCManager> m_npcManager;
    std::unique_ptr<TimerManager> m_timerManager;

    // 워커 스레드
    std::vector<std::unique_ptr<std::thread>> m_workerThreads;
    std::atomic<bool> m_running;
};
