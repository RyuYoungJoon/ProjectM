#include "MapManager.h"
#include "..\..\iocp_single\iocp_single\Protocol.h"
#include <cstdio>
#include <iostream>

MapManager::MapManager()
{
    memset(m_map, 0, sizeof(m_map));
}

MapManager::~MapManager()
{
}

bool MapManager::Initialize()
{
    if (!m_tileTexture.loadFromFile(Resources::TILE_TEXTURE_PATH))
    {
        std::cout << "Failed to load tile texture!" << std::endl;
        return false;
    }
    
    m_blankTile = GameObject(m_tileTexture, 0, 0, TILE_SIZE, TILE_SIZE);
    m_blockedTile = GameObject(m_tileTexture, TILE_SIZE, 0, TILE_SIZE, TILE_SIZE);
    
    return true;
}

void MapManager::LoadMapData(const char* filename)
{
    char data;
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "rb");
    
    if (fp == nullptr)
    {
        std::cout << "Failed to open map data file: " << filename << std::endl;
        return;
    }
    
    int count = 0;
    while (fscanf_s(fp, "%c", &data, 1) != EOF)
    {
        switch (data)
        {
        case '0':
            m_map[count / GameConfig::WORLD_WIDTH][count % GameConfig::WORLD_WIDTH] = eBLANK;
            count++;
            break;
        case '3':
            m_map[count / GameConfig::WORLD_WIDTH][count % GameConfig::WORLD_WIDTH] = eBLOCKED;
            count++;
            break;
        default:
            break;
        }
    }
    
    fclose(fp);
    std::cout << "Map loaded successfully. Tiles: " << count << std::endl;
}

void MapManager::Render(sf::RenderWindow* window, int leftX, int topY)
{
    for (int i = 0; i < SCREEN_WIDTH; ++i)
    {
        for (int j = 0; j < SCREEN_HEIGHT; ++j)
        {
            int tileX = i + leftX;
            int tileY = j + topY;
            
            // Map range check
            if (tileX < 0 || tileY < 0 || tileX >= GameConfig::WORLD_WIDTH || tileY >= GameConfig::WORLD_HEIGHT)
                continue;
            
            float posX = static_cast<float>(TILE_SIZE * i);
            float posY = static_cast<float>(TILE_SIZE * j);
            
            if (m_map[tileY][tileX] == eBLANK)
            {
                m_blankTile.SetPosition(posX, posY);
                m_blankTile.DrawSimple(window);
            }
            else if (m_map[tileY][tileX] == eBLOCKED)
            {
                m_blockedTile.SetPosition(posX, posY);
                m_blockedTile.DrawSimple(window);
            }
        }
    }
}

char MapManager::GetTile(int x, int y) const
{
    if (x < 0 || y < 0 || x >= GameConfig::WORLD_WIDTH || y >= GameConfig::WORLD_HEIGHT)
        return eBLOCKED;
    
    return m_map[y][x];
}

bool MapManager::IsBlocked(int x, int y) const
{
    return GetTile(x, y) == eBLOCKED;
}
