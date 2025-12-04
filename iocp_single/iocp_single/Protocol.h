#pragma once
#include "GameConfig.h"

enum GRID_TYPE
{
    eBLANK,
    eSTART,
    eEND,
    eBLOCKED,
    eOPEN,
    eCLOSE,
};

struct MAP
{
    GRID_TYPE type;
};

// 패킷 타입 정의
const char CS_PACKET_LOGIN = 1;
const char CS_PACKET_MOVE = 2;
const char CS_PACKET_ATTACK = 3;
const char CS_PACKET_CHAT = 4;
const char CS_PACKET_TELEPORT = 5;
const char CS_PACKET_LOGOUT = 6;

const char SC_PACKET_LOGIN_OK = 1;
const char SC_PACKET_MOVE = 2;
const char SC_PACKET_PUT_OBJECT = 3;
const char SC_PACKET_REMOVE_OBJECT = 4;
const char SC_PACKET_CHAT = 5;
const char SC_PACKET_LOGIN_FAIL = 6;
const char SC_PACKET_STATUS_CHANGE = 7;
const char SC_PACKET_ATTACK = 8;
const char SC_PACKET_STATUS_MONSTER = 9;
const char SC_PACKET_TELEPORT = 10;

#pragma pack(push, 1)

struct cs_packet_login {
    unsigned char size;
    char type;
    char userID[GameConfig::MAX_NAME_SIZE];
    int userPassWord;
};

struct cs_packet_move {
    unsigned char size;
    char type;
    char direction;
    int move_time;
};

struct cs_packet_attack {
    unsigned char size;
    char type;
};

struct cs_packet_chat {
    unsigned char size;
    char type;
    char message[GameConfig::MAX_CHAT_SIZE];
    int id;
};

struct cs_packet_logout {
    char size;
    char type;
};

struct cs_packet_teleport {
    unsigned char size;
    char type;
};

struct sc_packet_login_ok {
    unsigned char size;
    char type;
    int id;
    short x, y;
    short level;
    short hp, maxhp;
    int exp;
};

struct sc_packet_move {
    unsigned char size;
    char type;
    int id;
    char direction;
    short x, y;
    int move_time;
};

struct sc_packet_put_object {
    unsigned char size;
    char type;
    int id;
    char name[GameConfig::MAX_NAME_SIZE];
    short x, y;
    char object_type;
    char npcCharacterType;
    char npcMoveType;
};

struct sc_packet_remove_object {
    unsigned char size;
    char type;
    int id;
    bool isDie;
};

struct sc_packet_chat {
    unsigned char size;
    char type;
    int id;
    char mess[100];
    int chatType;
};

struct sc_packet_login_fail {
    unsigned char size;
    char type;
    int reason;
};

struct sc_packet_attack {
    char size;
    char type;
    char id;
};

struct sc_packet_status_change {
    unsigned char size;
    char type;
    short level;
    short hp, maxhp;
    int exp, max_exp;
    int id;
};

struct sc_packet_teleport {
    unsigned char size;
    char type;
    int x;
    int y;
};

struct sc_packet_status_monster {
    char size;
    char type;
    short c_hp;
    int id;
};

#pragma pack(pop)
