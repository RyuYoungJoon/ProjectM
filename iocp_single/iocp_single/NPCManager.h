#pragma once
#include "pch.h"

// Forward declarations
class Client;
class Sender;
class TimerManager;
struct MAP;

class NPCManager {
public:
    NPCManager(
        std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& clients,
        Sender& sender,
        TimerManager& timerManager,
        MAP (&gameMap)[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH]
    );

    void InitializeNPCs();
    void MoveNPC(int npcId);
    void WakeUpNPC(int npcId);
    void UpdateNPCInfo(int npcId, int playerId);

    bool IsNear(int id1, int id2) const;
    bool IsNPC(int id) const;
    bool IsPlayer(int id) const;

    // Lua API 함수들
    static int API_SendMessage(lua_State* L);
    static int API_get_x(lua_State* L);
    static int API_get_y(lua_State* L);
    static int API_npc_attack(lua_State* L);

private:
    bool CAS(volatile std::atomic_bool* addr, bool expected, bool newVal);

    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& m_clients;
    Sender& m_sender;
    TimerManager& m_timerManager;
    MAP (&m_gameMap)[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH];

    // Lua API에서 접근하기 위한 static 포인터
    static NPCManager* s_instance;
};
