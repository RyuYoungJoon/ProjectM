#pragma once
#define SFML_STATIC
#include <SFML/Graphics.hpp>

// Screen Settings
constexpr int TILE_SIZE = 32;
constexpr int SCREEN_WIDTH = 25;
constexpr int SCREEN_HEIGHT = 19;
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 608;

// UI Settings
constexpr int UI_PANEL_HEIGHT = 120;
constexpr int CHAT_BOX_HEIGHT = 150;
constexpr int HP_BAR_WIDTH = 80;
constexpr int HP_BAR_HEIGHT = 8;
constexpr int LEVEL_CIRCLE_RADIUS = 15;

// Game Settings
constexpr int BUF_SIZE = 200;
constexpr int FPS = 60;
constexpr int FRAME_TIME_MS = 1000 / FPS;

// Character Settings
constexpr float PLAYER_SPRITE_SCALE = 2.5f;
constexpr float NPC_SPRITE_SCALE = 2.0f;
constexpr int SPRITE_SIZE = 31;  // resize.png uses 31x31 sprites
constexpr int SPRITE_FRAMES = 10;  // 10 frames x 4 directions

// Font Settings
constexpr int FONT_SIZE_SMALL = 14;
constexpr int FONT_SIZE_NORMAL = 18;
constexpr int FONT_SIZE_LARGE = 24;
constexpr int FONT_SIZE_XLARGE = 32;

// Timeout Settings
constexpr int CHAT_TIMEOUT_SEC = 3;
constexpr int WORLD_CHAT_TIMEOUT_SEC = 5;
constexpr int RESPAWN_TEXT_TIMEOUT_SEC = 2;

// Color Settings
namespace Colors {
    const sf::Color PLAYER_NAME(255, 255, 0);
    const sf::Color HP_BACKGROUND(255, 0, 0, 100);
    const sf::Color HP_FOREGROUND(0, 255, 0, 100);
    const sf::Color EXP_BACKGROUND(0, 0, 0);
    const sf::Color EXP_FOREGROUND(0, 120, 255);
    const sf::Color LEVEL_BOX(0, 0, 0);
    const sf::Color LEVEL_BORDER(255, 255, 255);
}

// Resource Paths
namespace Resources {
    const char* const FONT_PATH = "NanumGothicBold.ttf";
    const char* const MAP_DATA_PATH = "mapData.txt";
    const char* const TILE_TEXTURE_PATH = "mapTile.bmp";
    const char* const PACMAN_TEXTURE_PATH = "pacman1.png";
    const char* const PLAYER_TEXTURE_PATH = "pngegg.png";
    const char* const PACMAN_ATTACK_TEXTURE_PATH = "pacmanAttack.png";
    const char* const PLAYER_ATTACK_TEXTURE_PATH = "player_Attack1.png";
}
