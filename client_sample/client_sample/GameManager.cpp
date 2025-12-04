#define _CRT_SECURE_NO_WARNINGS
#include "GameManager.h"
#include "..\..\iocp_single\iocp_single\Protocol.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <windows.h>

GameManager::GameManager()
    : m_myId(-1)
    , m_leftX(0)
    , m_topY(0)
    , m_gameStarted(false)
    , m_inPacketSize(0)
    , m_savedPacketSize(0)
{
    memset(m_packetBuffer, 0, sizeof(m_packetBuffer));
}

GameManager::~GameManager()
{
    Shutdown();
}

bool GameManager::Initialize()
{
    std::wcout.imbue(std::locale("korean"));
    
    // Create window FIRST
    m_window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D Pixel RPG Client - Refactored");
    m_window.setFramerateLimit(FPS);
    
    // Initialize renderer BEFORE login
    if (!m_renderManager.Initialize(&m_window))
    {
        std::cout << "Render manager initialization failed!" << std::endl;
        return false;
    }
    
    // Setup player textures BEFORE login
    m_player.SetMoveTexture(m_renderManager.GetPlayerTexture());
    m_player.SetAttackTexture(m_renderManager.GetPlayerAttackTexture());
    m_player.SetDamageTexture(m_renderManager.GetPacmanAttackTexture());
    
    // Debug: Print texture sizes
    auto playerTexSize = m_renderManager.GetPlayerTexture().getSize();
    auto pacmanTexSize = m_renderManager.GetPacmanTexture().getSize();
    std::cout << "Player texture: " << playerTexSize.x << "x" << playerTexSize.y << std::endl;
    std::cout << "Pacman texture: " << pacmanTexSize.x << "x" << pacmanTexSize.y << std::endl;
    std::cout << "Expected player: 310x124 (31x31 sprites, 10 frames, 4 directions)" << std::endl;
    std::cout << "Expected pacman: 310x155 (31x31 sprites, 10 frames, 5 types)" << std::endl;
    
    // Connect to server
    if (!m_networkManager.Connect("127.0.0.1", GameConfig::SERVER_PORT))
    {
        std::wcout << L"Server connection failed!" << std::endl;
        return false;
    }
    
    // Initialize map
    if (!m_mapManager.Initialize())
    {
        std::cout << "Map initialization failed!" << std::endl;
        return false;
    }
    
    m_mapManager.LoadMapData(Resources::MAP_DATA_PATH);
    
    // Login (now textures are already set!)
    if (!Login())
    {
        std::cout << "Login failed!" << std::endl;
        return false;
    }
    
    std::cout << "Game initialized successfully!" << std::endl;
    return true;
}

bool GameManager::Login()
{
    std::string userId;
    int password;
    
    std::cout << "ID: ";
    std::cin >> userId;
    
    std::cout << "Password: ";
    std::cin >> password;
    
    m_networkManager.SendLoginPacket(userId, password);
    
    // Wait for login response
    std::cout << "Waiting for login response..." << std::endl;
    
    DWORD startTime = timeGetTime();
    while (timeGetTime() - startTime < 5000) // 5 second timeout
    {
        char netBuf[200];
        size_t received = 0;
        
        if (m_networkManager.Receive(netBuf, 200, received))
        {
            if (received > 0)
            {
                ProcessData(netBuf, received);
                if (m_gameStarted)
                {
                    std::cout << "Login successful!" << std::endl;
                    return true;
                }
            }
        }
        Sleep(100);
    }
    
    std::cout << "Login timeout!" << std::endl;
    return false;
}

void GameManager::Run()
{
    DWORD lastTick = timeGetTime();
    
    while (m_window.isOpen())
    {
        DWORD currentTick = timeGetTime();
        if ((currentTick - lastTick) < FRAME_TIME_MS)
            continue;
        
        lastTick = currentTick;
        
        ProcessInput();
        ProcessNetwork();
        Update();
        Render();
    }
}

void GameManager::Shutdown()
{
    m_window.close();
    m_networkManager.Disconnect();
    m_renderManager.Shutdown();
}

void GameManager::ProcessInput()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            m_window.close();
        }
        
        if (event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
            case sf::Keyboard::Left:
                m_networkManager.SendMovePacket(GameConfig::D_LEFT);
                m_player.direction = 2;
                break;
                
            case sf::Keyboard::Right:
                m_networkManager.SendMovePacket(GameConfig::D_RIGHT);
                m_player.direction = 3;
                break;
                
            case sf::Keyboard::Up:
                m_networkManager.SendMovePacket(GameConfig::D_UP);
                m_player.direction = 0;
                break;
                
            case sf::Keyboard::Down:
                m_networkManager.SendMovePacket(GameConfig::D_DOWN);
                m_player.direction = 1;
                break;
                
            case sf::Keyboard::Space:
                m_networkManager.SendAttackPacket();
                m_player.attackFlag = true;
                break;
                
            case sf::Keyboard::A:
                m_networkManager.SendTeleportPacket();
                break;
                
            case sf::Keyboard::Escape:
                m_window.close();
                break;
            }
        }
    }
}

void GameManager::ProcessNetwork()
{
    char netBuf[200];
    size_t received = 0;
    
    if (!m_networkManager.Receive(netBuf, 200, received))
    {
        m_window.close();
        return;
    }
    
    if (received > 0)
    {
        ProcessData(netBuf, received);
    }
}

void GameManager::ProcessData(char* netBuf, size_t ioBytes)
{
    char* ptr = netBuf;
    
    while (ioBytes > 0)
    {
        if (m_inPacketSize == 0)
            m_inPacketSize = ptr[0];
        
        if (ioBytes + m_savedPacketSize >= m_inPacketSize)
        {
            memcpy(m_packetBuffer + m_savedPacketSize, ptr, m_inPacketSize - m_savedPacketSize);
            ProcessPacket(m_packetBuffer);
            
            ptr += m_inPacketSize - m_savedPacketSize;
            ioBytes -= m_inPacketSize - m_savedPacketSize;
            m_inPacketSize = 0;
            m_savedPacketSize = 0;
        }
        else
        {
            memcpy(m_packetBuffer + m_savedPacketSize, ptr, ioBytes);
            m_savedPacketSize += ioBytes;
            ioBytes = 0;
        }
    }
}

void GameManager::Update()
{
    UpdateCamera();
    m_player.UpdateAnimation();
    
    for (auto& npc : m_npcs)
    {
        npc.second.UpdateAnimation();
    }
}

void GameManager::UpdateCamera()
{
    m_leftX = m_player.m_x - (SCREEN_WIDTH / 2);
    m_topY = m_player.m_y - (SCREEN_HEIGHT / 2);
}

void GameManager::Render()
{
    m_window.clear();
    
    // Render map
    m_mapManager.Render(&m_window, m_leftX, m_topY);
    
    // Render player
    m_player.Draw(&m_window, m_leftX, m_topY, true, m_myId);
    
    // Render NPCs
    for (auto& npc : m_npcs)
    {
        npc.second.Draw(&m_window, m_leftX, m_topY, false, npc.first);
    }
    
    // Render UI
    m_renderManager.DrawPlayerInfo(m_player, m_leftX, m_topY);
    m_renderManager.RenderUI();
    
    m_window.display();
}
