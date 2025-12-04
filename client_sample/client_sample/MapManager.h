#pragma once
#include <SFML/Graphics.hpp>
#include "GameObject.h"
#include "Config.h"

// Protocol.h will define WORLD_WIDTH, WORLD_HEIGHT, eBLANK, eBLOCKED

class MapManager {
private:
    char m_map[800][800];  // Use literals to avoid conflict
    GameObject m_blankTile;
    GameObject m_blockedTile;
    sf::Texture m_tileTexture;
    
public:
    MapManager();
    ~MapManager();
    
    bool Initialize();
    void LoadMapData(const char* filename);
    void Render(sf::RenderWindow* window, int leftX, int topY);
    
    char GetTile(int x, int y) const;
    bool IsBlocked(int x, int y) const;
};
