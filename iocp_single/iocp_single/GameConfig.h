#pragma once

namespace GameConfig {
    // 서버 설정
    constexpr short SERVER_PORT = 4000;
    constexpr int WORKER_THREAD_COUNT = 6;

    // 월드 설정
    constexpr int WORLD_HEIGHT = 800;
    constexpr int WORLD_WIDTH = 800;
    constexpr int VIEW_RANGE = 9;

    // 플레이어 설정
    constexpr int MAX_USER = 10000;
    constexpr int MAX_NAME_SIZE = 20;
    constexpr int MAX_CHAT_SIZE = 100;

    // NPC 설정
    constexpr int MAX_NPC = 10000;
    constexpr int NPC_ID_START = MAX_USER;
    constexpr int NPC_ID_END = MAX_USER + MAX_NPC - 1;

    // 전투 설정
    namespace Combat {
        constexpr short BASE_HP = 100;
        constexpr short HP_PER_LEVEL = 20;
        constexpr short BASE_DAMAGE = 20;
        constexpr float HEAL_RATE = 0.1f;
        constexpr int HEAL_INTERVAL_SEC = 5;
        constexpr int ATTACK_COOLDOWN_SEC = 1;
    }

    // 경험치 설정
    namespace Experience {
        constexpr int BASE_EXP = 100;
        inline int GetRequiredExp(int level) {
            return static_cast<int>(pow(2, level - 1) * BASE_EXP);
        }
    }

    // 패킷 설정
    constexpr int MAX_PACKET_SIZE = 255;
    constexpr int MAX_BUF_SIZE = 1024;
    constexpr int BUFSIZE = 256;

    // 방향
    constexpr unsigned char D_UP = 0;
    constexpr unsigned char D_DOWN = 1;
    constexpr unsigned char D_LEFT = 2;
    constexpr unsigned char D_RIGHT = 3;

    // NPC 타입
    constexpr char NPC_STAY = 0;
    constexpr char NPC_FIGHT = 1;
    constexpr char NPC_SPECIAL = 2;

    constexpr char NPC_HOLD = 0;
    constexpr char NPC_RANDOM_MOVE = 1;
}
