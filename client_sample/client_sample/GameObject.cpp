#include "GameObject.h"
#include "..\..\iocp_single\iocp_single\Protocol.h"
#include <cstring>

GameObject::GameObject()
    : m_showing(false)
    , m_x(0)
    , m_y(0)
    , m_id(0)
    , level(1)
    , exp(0)
    , hp(100.0f)
    , maxHp(100.0f)
    , motionIndex(0)
    , attackIndex(0)
    , damageIndex(0)
    , direction(1)
    , attackFlag(false)
    , damageFlag(false)
    , npcCharacterType(0)
    , npcMoveType(0)
{
    m_timeOut = std::chrono::high_resolution_clock::now();
    memset(name, 0, sizeof(name));
    memset(m_message, 0, sizeof(m_message));
}

GameObject::GameObject(sf::Texture& texture, int x, int y, int width, int height)
    : GameObject()
{
    m_sprite.setTexture(texture);
    m_sprite.setTextureRect(sf::IntRect(x, y, width, height));
}

void GameObject::Show()
{
    if (hp > 0)
        m_showing = true;
}

void GameObject::Hide()
{
    m_showing = false;
}

void GameObject::Move(int x, int y)
{
    m_x = x;
    m_y = y;
}

void GameObject::SetPosition(float x, float y)
{
    m_sprite.setPosition(x, y);
}

void GameObject::SetMoveTexture(sf::Texture& texture)
{
    m_sprite.setTexture(texture);
}

void GameObject::SetAttackTexture(sf::Texture& texture)
{
    m_attackSprite.setTexture(texture);
    m_attackSprite.setTextureRect(sf::IntRect(0, 0, 192, 178));
}

void GameObject::SetDamageTexture(sf::Texture& texture)
{
    m_damageSprite.setTexture(texture);
    m_damageSprite.setTextureRect(sf::IntRect(0, 0, 192, 178));
}

void GameObject::SetHp(float currentHp, float maximum)
{
    hp = currentHp;
    maxHp = maximum;
}

void GameObject::SetName(const char* str, const sf::Font& font)
{
    m_nameText.setFont(font);
    m_nameText.setString(str);
    m_nameText.setCharacterSize(FONT_SIZE_NORMAL);
    m_nameText.setFillColor(Colors::PLAYER_NAME);
    m_nameText.setStyle(sf::Text::Bold);
    strcpy_s(name, str);
}

void GameObject::SetLevel(int lv, const sf::Font& font)
{
    level = lv;
    char buffer[20];
    sprintf_s(buffer, "%d", lv);
    
    m_levelText.setFont(font);
    m_levelText.setString(buffer);
    m_levelText.setCharacterSize(FONT_SIZE_NORMAL);
    m_levelText.setFillColor(sf::Color::White);
    m_levelText.setStyle(sf::Text::Bold);
}

void GameObject::SetExp(int experience, const sf::Font& font)
{
    exp = experience;
    char buffer[50];
    sprintf_s(buffer, "EXP: %d", experience);
    
    m_expText.setFont(font);
    m_expText.setString(buffer);
    m_expText.setCharacterSize(FONT_SIZE_LARGE);
    m_expText.setFillColor(sf::Color::Yellow);
    m_expText.setStyle(sf::Text::Bold);
}

void GameObject::AddChat(const char* chat, const sf::Font& font)
{
    m_chatText.setFont(font);
    m_chatText.setString(chat);
    m_chatText.setCharacterSize(FONT_SIZE_SMALL);
    m_chatText.setFillColor(sf::Color::White);
    m_timeOut = std::chrono::high_resolution_clock::now() + std::chrono::seconds(CHAT_TIMEOUT_SEC);
}

void GameObject::Draw(sf::RenderWindow* window, int leftX, int topY, bool isPlayer, int myId)
{
    if (!m_showing) return;
    
    float rx = (m_x - leftX) * static_cast<float>(TILE_SIZE) + 8.0f;
    float ry = (m_y - topY) * static_cast<float>(TILE_SIZE) + 8.0f;
    
    // Character body rendering
    if (isPlayer || m_id < GameConfig::NPC_ID_START)
    {
        int frameX = motionIndex * SPRITE_SIZE;
        int frameY = direction * SPRITE_SIZE;
        
        m_sprite.setTextureRect(sf::IntRect(frameX, frameY, SPRITE_SIZE, SPRITE_SIZE));
        m_sprite.setPosition(rx - 20, ry - 35);
        m_sprite.setScale(PLAYER_SPRITE_SCALE, PLAYER_SPRITE_SCALE);
    }
    else
    {
        m_sprite.setPosition(rx, ry);
        m_sprite.setScale(NPC_SPRITE_SCALE, NPC_SPRITE_SCALE);
    }
    window->draw(m_sprite);
    
    // Attack effect
    if (attackFlag)
    {
        int frameX = (attackIndex / 3) * 96;
        m_attackSprite.setTextureRect(sf::IntRect(frameX, 0, 96, 96));
        m_attackSprite.setScale(0.8f, 0.8f);
        
        m_attackSprite.setPosition(rx - 10, ry - 70);
        window->draw(m_attackSprite);
        
        attackIndex++;
        if (attackIndex >= 9)
        {
            attackIndex = 0;
            attackFlag = false;
        }
    }
    
    // Damage effect
    if (damageFlag)
    {
        int frameX = (damageIndex % 5) * 130;
        m_damageSprite.setTextureRect(sf::IntRect(frameX, 0, 130, 210));
        m_damageSprite.setScale(0.7f, 0.7f);
        m_damageSprite.setPosition(rx - 25, ry - 80);
        window->draw(m_damageSprite);
        
        damageIndex++;
        if (damageIndex >= 5)
        {
            damageIndex = 0;
            damageFlag = false;
        }
    }
    
    // Name display
    m_nameText.setPosition(rx, ry - 55);
    window->draw(m_nameText);
    
    // Level display
    m_levelCircle.setPosition(sf::Vector2f(rx - 35, ry - 35));
    m_levelCircle.setOutlineThickness(2.0f);
    m_levelCircle.setOutlineColor(Colors::LEVEL_BORDER);
    m_levelCircle.setFillColor(Colors::LEVEL_BOX);
    m_levelCircle.setRadius(static_cast<float>(LEVEL_CIRCLE_RADIUS));
    window->draw(m_levelCircle);
    
    m_levelText.setPosition(rx - 28, ry - 35);
    window->draw(m_levelText);
    
    // HP bar display (for other characters)
    if ((m_id >= GameConfig::NPC_ID_START || m_id < GameConfig::NPC_ID_START) && m_id != myId)
    {
        sf::RectangleShape hpBox;
        hpBox.setPosition(sf::Vector2f(rx, ry - 35));
        hpBox.setOutlineThickness(2.0f);
        hpBox.setOutlineColor(sf::Color::White);
        hpBox.setFillColor(Colors::HP_BACKGROUND);
        hpBox.setSize(sf::Vector2f(static_cast<float>(HP_BAR_WIDTH), static_cast<float>(HP_BAR_HEIGHT)));
        window->draw(hpBox);
        
        sf::RectangleShape hpBar;
        hpBar.setPosition(sf::Vector2f(rx, ry - 35));
        hpBar.setFillColor(Colors::HP_FOREGROUND);
        float hpRatio = (maxHp > 0) ? (hp / maxHp) : 0.0f;
        hpBar.setSize(sf::Vector2f(hpRatio * HP_BAR_WIDTH, static_cast<float>(HP_BAR_HEIGHT)));
        window->draw(hpBar);
        
        char hpBuffer[50];
        sprintf_s(hpBuffer, "%.0f/%.0f", hp, maxHp);
        m_hpText.setString(hpBuffer);
        m_hpText.setCharacterSize(10);
        m_hpText.setFillColor(sf::Color::White);
        m_hpText.setPosition(rx + 5, ry - 35);
        window->draw(m_hpText);
    }
    
    // Chat message display
    if (std::chrono::high_resolution_clock::now() < m_timeOut)
    {
        m_chatText.setPosition(rx - 10, ry + 15);
        window->draw(m_chatText);
    }
}

void GameObject::DrawSimple(sf::RenderWindow* window)
{
    window->draw(m_sprite);
}

void GameObject::UpdateAnimation()
{
    motionIndex = (motionIndex + 1) % SPRITE_FRAMES;
}
