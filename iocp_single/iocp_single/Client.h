#pragma once
#include "enum.h"
#include "GameConfig.h"

class error;

class Client
{
public:
    // Basic Info
    char name[GameConfig::MAX_NAME_SIZE];
    int _id;
    short x, y;
    
    // View Management
    std::unordered_set<int> viewlist;
    std::mutex vl;
    std::mutex cl;
    
    // Lua State
    lua_State* L;

    // Stats
    short level;
    short hp;
    short maxhp;
    int exp;
    short damage;
    
    // State
    std::mutex state_lock;
    STATUS _state;
    std::atomic_bool _is_active;
    int _type;  // 1: Player, 2: NPC
    
    // Movement
    int move_count;
    bool overlap;
    char direction;
    int last_move_time;

    // NPC Type
    char npcCharacterType;  // 0: stay, 1: fight, 2: special
    char npcMoveType;       // 0: hold, 1: moving, 2: special
    
    // Combat
    bool can_attack;

    // Network
    Exp_Over _recv_over;
    SOCKET _socket;
    int _prev_size;

public:
    Client();
    ~Client();
};
