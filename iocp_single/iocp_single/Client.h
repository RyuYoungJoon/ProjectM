#pragma once
#include "enum.h"
#include "GameConfig.h"

class error;

class Client
{
public:
    // 기본 정보
    char name[GameConfig::MAX_NAME_SIZE];
    int _id;
    short x, y;
    
    // 시야 관리
    std::unordered_set<int> viewlist;
    std::mutex vl;
    std::mutex cl;
    
    // Lua 상태
    lua_State* L;

    // 스탯
    short level;
    short hp;
    short maxhp;
    int exp;
    short damage;
    
    // 상태
    std::mutex state_lock;
    STATUS _state;
    std::atomic_bool _is_active;
    int _type;  // 1: Player, 2: NPC
    
    // 이동 관련
    int move_count;
    bool overlap;
    char direction;
    int last_move_time;

    // NPC 타입
    char npcCharacterType;  // 0: stay, 1: fight, 2: special
    char npcMoveType;       // 0: hold, 1: moving, 2: special
    
    // 전투
    bool can_attack;

    // 네트워크
    Exp_Over _recv_over;
    SOCKET _socket;
    int _prev_size;

public:
    Client();
    ~Client();
};
