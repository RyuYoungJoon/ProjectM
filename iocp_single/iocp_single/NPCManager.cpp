#include "pch.h"
#include "NPCManager.h"
#include "Client.h"
#include "Sender.h"
#include "TimerManager.h"

NPCManager* NPCManager::s_instance = nullptr;

NPCManager::NPCManager(
    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& clients,
    Sender& sender,
    TimerManager& timerManager,
    MAP (&gameMap)[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH]
)
    : m_clients(clients)
    , m_sender(sender)
    , m_timerManager(timerManager)
    , m_gameMap(gameMap)
{
    s_instance = this;
}

bool NPCManager::IsNear(int id1, int id2) const {
    if (GameConfig::VIEW_RANGE < abs(m_clients[id1].x - m_clients[id2].x)) return false;
    if (GameConfig::VIEW_RANGE < abs(m_clients[id1].y - m_clients[id2].y)) return false;
    return true;
}

bool NPCManager::IsNPC(int id) const {
    return (id >= GameConfig::NPC_ID_START) && (id <= GameConfig::NPC_ID_END);
}

bool NPCManager::IsPlayer(int id) const {
    return (id >= 0) && (id < GameConfig::MAX_USER);
}

bool NPCManager::CAS(volatile std::atomic_bool* addr, bool expected, bool newVal) {
    return std::atomic_compare_exchange_strong(
        reinterpret_cast<volatile std::atomic_bool*>(addr), 
        &expected, 
        newVal
    );
}

void NPCManager::WakeUpNPC(int id) {
    if (false == m_clients[id]._is_active) {
        if (true == CAS(&m_clients[id]._is_active, false, true)) {
            m_timerManager.AddTimer(id, EVENT_NPC_MOVE, std::chrono::system_clock::now() + std::chrono::seconds(1));
        }
    }
}

void NPCManager::InitializeNPCs() {
    using namespace GameConfig;
    
    for (int i = NPC_ID_START; i <= NPC_ID_END; ++i) {
        sprintf_s(m_clients[i].name, "N%d", i);
        m_clients[i]._id = i;
        m_clients[i]._state = STATUS::ST_INGAME;
        m_clients[i]._type = 2;

        // Find empty space
        while (true) {
            int x = rand() % WORLD_WIDTH;
            int y = rand() % WORLD_HEIGHT;

            if (m_gameMap[y][x].type == eBLANK) {
                m_clients[i].x = x;
                m_clients[i].y = y;
                break;
            }
        }

        // Lua initialization
        lua_State* L = m_clients[i].L = luaL_newstate();
        luaL_openlibs(L);
        int error = luaL_loadfile(L, "monster.lua") || lua_pcall(L, 0, 0, 0);

        lua_getglobal(L, "set_uid");
        lua_pushnumber(L, i);
        lua_pcall(L, 1, 1, 0);

        // Register Lua API
        lua_register(L, "API_SendMessage", API_SendMessage);
        lua_register(L, "API_get_x", API_get_x);
        lua_register(L, "API_get_y", API_get_y);
        lua_register(L, "API_npc_attack", API_npc_attack);

        // Initialize NPC stats
        m_clients[i].level = rand() % 5 + 1;
        m_clients[i].npcCharacterType = rand() % 3;
        m_clients[i].hp = m_clients[i].level * 5;
        m_clients[i].maxhp = m_clients[i].hp;

        if (m_clients[i].npcCharacterType == GameConfig::NPC_SPECIAL) {
            m_clients[i].npcMoveType = GameConfig::NPC_SPECIAL;
        } else {
            m_clients[i].npcMoveType = rand() % 2;
        }

        m_clients[i]._is_active = false;
        m_clients[i].overlap = false;
        m_clients[i].move_count = 0;

        // NPCs start inactive and are woken up only when players are nearby
        // This prevents timer flood on server startup
    }

    std::cout << "Initialize NPC Complete" << std::endl;
}

void NPCManager::MoveNPC(int npcId) {
    using namespace GameConfig;
    
    Client& npc = m_clients[npcId];
    std::unordered_set<int> oldViewlist = npc.viewlist;
    std::unordered_set<int> newViewlist;

    // Find nearby players
    for (auto& obj : m_clients) {
        if (obj._state != STATUS::ST_INGAME) continue;
        if (!IsPlayer(obj._id)) continue;
        if (IsNear(npcId, obj._id)) {
            oldViewlist.insert(obj._id);
        }
    }

    // Move NPC
    int x = npc.x;
    int y = npc.y;

    switch (rand() % 4) {
    case D_UP:
        if (y > 0 && m_gameMap[y - 1][x].type == eBLANK) {
            y--;
            npc.direction = D_UP;
        }
        break;
    case D_DOWN:
        if (y < (WORLD_HEIGHT - 1) && m_gameMap[y + 1][x].type == eBLANK) {
            y++;
            npc.direction = D_DOWN;
        }
        break;
    case D_LEFT:
        if (x > 0 && m_gameMap[y][x - 1].type == eBLANK) {
            x--;
            npc.direction = D_LEFT;
        }
        break;
    case D_RIGHT:
        if (x < (WORLD_WIDTH - 1) && m_gameMap[y][x + 1].type == eBLANK) {
            x++;
            npc.direction = D_RIGHT;
        }
        break;
    }

    npc.x = x;
    npc.y = y;

    // Build new viewlist
    for (auto& obj : m_clients) {
        if (obj._state != STATUS::ST_INGAME) continue;
        if (!IsPlayer(obj._id)) continue;
        if (IsNear(npcId, obj._id)) {
            newViewlist.insert(obj._id);
        }
    }

    // Handle newly visible players
    for (auto pl : newViewlist) {
        if (oldViewlist.count(pl) == 0) {
            std::lock_guard<std::mutex> lock(m_clients[pl].vl);
            m_clients[pl].viewlist.insert(npcId);
            
            m_sender.send_put_object(m_clients[pl]._socket, pl,
                npcId, npc.x, npc.y, npc.name,
                npc.npcCharacterType, npc.npcMoveType);
            m_sender.send_status_change(m_clients[pl]._socket, pl,
                npcId, npc.hp, npc.maxhp, npc.level, npc.exp);
        } else {
            m_sender.send_move_packet(m_clients[pl]._socket, pl, npcId,
                npc.x, npc.y, npc.last_move_time, npc.direction);
        }
    }

    // Handle players out of view
    for (auto pl : oldViewlist) {
        if (newViewlist.count(pl) == 0) {
            std::lock_guard<std::mutex> lock(m_clients[pl].vl);
            m_clients[pl].viewlist.erase(npcId);
            m_sender.send_remove_object(m_clients[pl]._socket, pl, npcId, false);
        }
    }

    // Manage active state
    if (newViewlist.empty()) {
        CAS(&npc._is_active, true, false);
    } else {
        if (npc._state == STATUS::ST_INGAME) {
            m_timerManager.AddTimer(npcId, EVENT_NPC_MOVE, 
                std::chrono::system_clock::now() + std::chrono::seconds(1));
        }
    }
}

void NPCManager::UpdateNPCInfo(int npcId, int playerId) {
    using namespace GameConfig;
    
    m_clients[playerId].hp -= m_clients[npcId].damage;

    if (m_clients[playerId].hp <= 0) {
        m_clients[playerId].x = 100;
        m_clients[playerId].y = 100;
        m_clients[playerId].hp = m_clients[playerId].maxhp;
        if (m_clients[playerId].exp > 0) {
            m_clients[playerId].exp = m_clients[playerId].exp / 2;
        }
    }

    m_sender.send_status_change(m_clients[playerId]._socket, playerId,
        playerId, m_clients[playerId].hp, m_clients[playerId].maxhp,
        m_clients[playerId].level, m_clients[playerId].exp);

    for (auto other : m_clients[playerId].viewlist) {
        if (IsNPC(other)) continue;
        m_sender.send_status_change(m_clients[other]._socket, other,
            playerId, m_clients[playerId].hp, m_clients[playerId].maxhp,
            m_clients[playerId].level, m_clients[playerId].exp);
    }
}

// Lua API implementation
int NPCManager::API_SendMessage(lua_State* L) {
    int myId = (int)lua_tointeger(L, -3);
    int userId = (int)lua_tointeger(L, -2);
    char* mess = (char*)lua_tostring(L, -1);
    lua_pop(L, 4);
    return 0;
}

int NPCManager::API_get_x(lua_State* L) {
    int userId = (int)lua_tointeger(L, -1);
    lua_pop(L, 2);
    int x = s_instance->m_clients[userId].x;
    lua_pushnumber(L, x);
    return 1;
}

int NPCManager::API_get_y(lua_State* L) {
    int userId = (int)lua_tointeger(L, -1);
    lua_pop(L, 2);
    int y = s_instance->m_clients[userId].y;
    lua_pushnumber(L, y);
    return 1;
}

int NPCManager::API_npc_attack(lua_State* L) {
    int player = (int)lua_tointeger(L, -2);
    int npcId = (int)lua_tonumber(L, -1);
    lua_pop(L, 3);

    if (s_instance->m_clients[npcId].can_attack) {
        s_instance->m_clients[npcId].can_attack = false;
        std::cout << "NPC attack" << std::endl;
        s_instance->UpdateNPCInfo(npcId, player);
        s_instance->m_timerManager.AddTimer(npcId, EVENT_ATTACK, 
            std::chrono::system_clock::now() + std::chrono::seconds(1));
    }
    return 0;
}
