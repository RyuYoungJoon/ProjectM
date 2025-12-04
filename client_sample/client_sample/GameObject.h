#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>
#include "Config.h"

// Protocol.h will define MAX_STR_LEN and MAX_ID_LEN

class GameObject {
private:
    bool m_showing;
    char m_message[200];  // Use literal to avoid conflict
    std::chrono::high_resolution_clock::time_point m_timeOut;
    
    sf::Text m_chatText;
    sf::Text m_nameText;
    sf::Text m_levelText;
    sf::Text m_hpText;
    sf::Text m_expText;
    sf::CircleShape m_levelCircle;

public:
    // Sprites
    sf::Sprite m_sprite;
    sf::Sprite m_attackSprite;
    sf::Sprite m_damageSprite;
    
    // Position and ID
    int m_x, m_y;
    int m_id;
    
    // Stats
    char name[20];  // Use literal to avoid conflict
    short level;
    int exp;
    float hp;
    float maxHp;
    
    // Animation
    int motionIndex;
    int attackIndex;
    int damageIndex;
    char direction;
    
    // State Flags
    bool attackFlag;
    bool damageFlag;
    
    // NPC Type
    char npcCharacterType;
    char npcMoveType;

public:
    GameObject();
    GameObject(sf::Texture& texture, int x, int y, int width, int height);
    
    // Basic Functions
    void Show();
    void Hide();
    bool IsShowing() const { return m_showing; }
    
    // Position
    void Move(int x, int y);
    void SetPosition(float x, float y);
    
    // Texture
    void SetMoveTexture(sf::Texture& texture);
    void SetAttackTexture(sf::Texture& texture);
    void SetDamageTexture(sf::Texture& texture);
    
    // Stats
    void SetHp(float currentHp, float maximum);
    void SetName(const char* str, const sf::Font& font);
    void SetLevel(int lv, const sf::Font& font);
    void SetExp(int experience, const sf::Font& font);
    
    // Chat
    void AddChat(const char* chat, const sf::Font& font);
    
    // Rendering
    void Draw(sf::RenderWindow* window, int leftX, int topY, bool isPlayer, int myId);
    void DrawSimple(sf::RenderWindow* window);
    
    // Animation
    void UpdateAnimation();
};
