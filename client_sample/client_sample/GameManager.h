#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include "GameObject.h"
#include "MapManager.h"
#include "NetworkManager.h"
#include "RenderManager.h"
#include "Config.h"

// Protocol.h will define NPC_ID_START

class GameManager {
private:
    // Managers
    MapManager m_mapManager;
    NetworkManager m_networkManager;
    RenderManager m_renderManager;
    
    // Game Objects
    GameObject m_player;
    std::unordered_map<int, GameObject> m_npcs;
    
    // Game State
    int m_myId;
    int m_leftX, m_topY;
    bool m_gameStarted;
    
    // Window
    sf::RenderWindow m_window;
    
    // Packet Processing
    char m_packetBuffer[200];  // BUF_SIZE
    size_t m_inPacketSize;
    size_t m_savedPacketSize;
    
public:
    GameManager();
    ~GameManager();
    
    bool Initialize();
    void Run();
    void Shutdown();
    
private:
    // Game Loop
    void ProcessInput();
    void Update();
    void Render();
    
    // Network
    void ProcessNetwork();
    void ProcessData(char* netBuf, size_t ioBytes);
    void ProcessPacket(char* packet);
    
    // Camera
    void UpdateCamera();
    
    // Utility
    bool Login();
};
