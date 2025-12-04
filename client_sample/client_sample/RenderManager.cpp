#include "RenderManager.h"
#include <iostream>
#include <cmath>

RenderManager::RenderManager()
    : m_window(nullptr)
{
}

RenderManager::~RenderManager()
{
    Shutdown();
}

bool RenderManager::Initialize(sf::RenderWindow* window)
{
    m_window = window;
    
    // Load font
    if (!m_font.loadFromFile(Resources::FONT_PATH))
    {
        std::cout << "Font loading error!" << std::endl;
        return false;
    }
    
    // Load textures
    if (!m_playerTexture.loadFromFile(Resources::PLAYER_TEXTURE_PATH))
    {
        std::cout << "Failed to load player texture!" << std::endl;
        return false;
    }
    
    if (!m_pacmanTexture.loadFromFile(Resources::PACMAN_TEXTURE_PATH))
    {
        std::cout << "Failed to load pacman texture!" << std::endl;
        return false;
    }
    
    if (!m_playerAttackTexture.loadFromFile(Resources::PLAYER_ATTACK_TEXTURE_PATH))
    {
        std::cout << "Failed to load player attack texture!" << std::endl;
        return false;
    }
    
    if (!m_pacmanAttackTexture.loadFromFile(Resources::PACMAN_ATTACK_TEXTURE_PATH))
    {
        std::cout << "Failed to load pacman attack texture!" << std::endl;
        return false;
    }
    
    // Initialize world chat text
    for (int i = 0; i < 3; ++i)
    {
        m_worldText[i].setFont(m_font);
        m_worldText[i].setCharacterSize(FONT_SIZE_NORMAL);
        m_worldText[i].setFillColor(sf::Color::White);
        m_worldTextTimeout[i] = std::chrono::high_resolution_clock::now();
    }
    
    // Initialize respawn text
    m_respawnText.setFont(m_font);
    m_respawnText.setCharacterSize(FONT_SIZE_XLARGE * 2);
    m_respawnText.setFillColor(sf::Color::Red);
    m_respawnText.setStyle(sf::Text::Bold);
    m_respawnTextTimeout = std::chrono::high_resolution_clock::now();
    
    std::cout << "RenderManager initialized successfully!" << std::endl;
    return true;
}

void RenderManager::Shutdown()
{
}

void RenderManager::DrawPlayerInfo(const GameObject& player, int leftX, int topY)
{
    if (!m_window) return;
    
    // Player position info
    sf::Text posText;
    posText.setFont(m_font);
    char posBuf[100];
    sprintf_s(posBuf, "Position: (%d, %d)", player.m_x, player.m_y);
    posText.setString(posBuf);
    posText.setPosition(10, 10);
    posText.setCharacterSize(FONT_SIZE_LARGE);
    posText.setFillColor(sf::Color::White);
    m_window->draw(posText);
    
    // Player stat info
    sf::Text statText;
    statText.setFont(m_font);
    int maxExp = static_cast<int>(100 * pow(2, (player.level - 1)));
    sprintf_s(posBuf, "LV: %d | EXP: %d/%d | HP: %.0f/%.0f", 
              player.level, player.exp, maxExp, player.hp, player.maxHp);
    statText.setString(posBuf);
    statText.setPosition(10, 40);
    statText.setCharacterSize(FONT_SIZE_LARGE);
    statText.setFillColor(sf::Color::Yellow);
    statText.setStyle(sf::Text::Bold);
    m_window->draw(statText);
    
    // Player HP bar
    float rx = (player.m_x - leftX) * static_cast<float>(TILE_SIZE) + 8.0f;
    float ry = (player.m_y - topY) * static_cast<float>(TILE_SIZE) + 8.0f;
    
    m_hpBoxBg.setPosition(sf::Vector2f(rx, ry - 35));
    m_hpBoxBg.setOutlineThickness(2.0f);
    m_hpBoxBg.setOutlineColor(sf::Color::White);
    m_hpBoxBg.setFillColor(Colors::HP_BACKGROUND);
    m_hpBoxBg.setSize(sf::Vector2f(static_cast<float>(HP_BAR_WIDTH), static_cast<float>(HP_BAR_HEIGHT)));
    m_window->draw(m_hpBoxBg);
    
    m_hpBar.setPosition(sf::Vector2f(rx, ry - 35));
    m_hpBar.setFillColor(Colors::HP_FOREGROUND);
    float hpRatio = (player.maxHp > 0) ? (player.hp / player.maxHp) : 0.0f;
    m_hpBar.setSize(sf::Vector2f(hpRatio * HP_BAR_WIDTH, static_cast<float>(HP_BAR_HEIGHT)));
    m_window->draw(m_hpBar);
    
    // EXP bar background
    m_expBoxBg.setPosition(sf::Vector2f(10, static_cast<float>(WINDOW_HEIGHT - 30)));
    m_expBoxBg.setOutlineThickness(2.0f);
    m_expBoxBg.setOutlineColor(sf::Color::White);
    m_expBoxBg.setFillColor(Colors::EXP_BACKGROUND);
    m_expBoxBg.setSize(sf::Vector2f(static_cast<float>(WINDOW_WIDTH - 20), 20));
    m_window->draw(m_expBoxBg);
    
    // EXP bar
    if (player.level > 0)
    {
        m_expBar.setPosition(sf::Vector2f(10, static_cast<float>(WINDOW_HEIGHT - 30)));
        m_expBar.setFillColor(Colors::EXP_FOREGROUND);
        float expRatio = static_cast<float>(player.exp) / static_cast<float>(maxExp);
        m_expBar.setSize(sf::Vector2f(expRatio * (WINDOW_WIDTH - 20), 20));
        m_window->draw(m_expBar);
    }
}

void RenderManager::DrawWorldChat(const char* message, int index)
{
    if (index < 0 || index >= 3) return;
    
    // Move previous messages down
    if (index == 0)
    {
        m_worldText[2].setString(m_worldText[1].getString());
        m_worldText[1].setString(m_worldText[0].getString());
        
        m_worldTextTimeout[2] = m_worldTextTimeout[1];
        m_worldTextTimeout[1] = m_worldTextTimeout[0];
    }
    
    m_worldText[index].setString(message);
    m_worldTextTimeout[index] = std::chrono::high_resolution_clock::now() + 
                                  std::chrono::seconds(WORLD_CHAT_TIMEOUT_SEC);
}

void RenderManager::DrawRespawnText(const char* message)
{
    m_respawnText.setString(message);
    m_respawnTextTimeout = std::chrono::high_resolution_clock::now() + 
                            std::chrono::seconds(RESPAWN_TEXT_TIMEOUT_SEC);
}

void RenderManager::RenderUI()
{
    if (!m_window) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    
    // Render world chat
    for (int i = 0; i < 3; ++i)
    {
        if (now < m_worldTextTimeout[i])
        {
            m_worldText[i].setPosition(10, static_cast<float>(WINDOW_HEIGHT - 120 + (i * 30)));
            m_window->draw(m_worldText[i]);
        }
    }
    
    // Render respawn text
    if (now < m_respawnTextTimeout)
    {
        m_respawnText.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2);
        m_window->draw(m_respawnText);
    }
}
