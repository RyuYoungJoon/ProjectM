#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <chrono>
#include "GameObject.h"
#include "Config.h"

class RenderManager {
private:
    sf::RenderWindow* m_window;
    sf::Font m_font;
    
    // Textures
    sf::Texture m_pacmanTexture;
    sf::Texture m_playerTexture;
    sf::Texture m_pacmanAttackTexture;
    sf::Texture m_playerAttackTexture;
    
    // UI Text
    sf::Text m_worldText[3];
    std::chrono::high_resolution_clock::time_point m_worldTextTimeout[3];
    
    sf::Text m_respawnText;
    std::chrono::high_resolution_clock::time_point m_respawnTextTimeout;
    
    // UI Elements
    sf::RectangleShape m_expBoxBg;
    sf::RectangleShape m_expBar;
    sf::RectangleShape m_hpBoxBg;
    sf::RectangleShape m_hpBar;
    
public:
    RenderManager();
    ~RenderManager();
    
    bool Initialize(sf::RenderWindow* window);
    void Shutdown();
    
    // Texture Accessors
    sf::Texture& GetPlayerTexture() { return m_playerTexture; }
    sf::Texture& GetPacmanTexture() { return m_pacmanTexture; }
    sf::Texture& GetPlayerAttackTexture() { return m_playerAttackTexture; }
    sf::Texture& GetPacmanAttackTexture() { return m_pacmanAttackTexture; }
    sf::Font& GetFont() { return m_font; }
    
    // UI Rendering
    void DrawPlayerInfo(const GameObject& player, int leftX, int topY);
    void DrawWorldChat(const char* message, int index);
    void DrawRespawnText(const char* message);
    void RenderUI();
    
    // Utility
    sf::RenderWindow* GetWindow() { return m_window; }
};
