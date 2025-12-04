#define _CRT_SECURE_NO_WARNINGS
#include "GameManager.h"
#include "..\..\iocp_single\iocp_single\Protocol.h"
#include <iostream>
#include <cmath>

void GameManager::ProcessPacket(char* ptr)
{
    switch (ptr[1])
    {
    case SC_PACKET_LOGIN_OK:
    {
        m_gameStarted = true;
        sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
        
        std::cout << "SC_PACKET_LOGIN_OK - ID: " << packet->id << std::endl;
        
        m_myId = packet->id;
        m_player.m_id = packet->id;
        m_player.Move(packet->x, packet->y);
        m_player.hp = static_cast<float>(packet->hp);
        m_player.maxHp = static_cast<float>(packet->maxhp);
        m_player.exp = packet->exp;
        m_player.level = packet->level;
        
        m_leftX = packet->x - (SCREEN_WIDTH / 2);
        m_topY = packet->y - (SCREEN_HEIGHT / 2);
        
        // Set player name, level, exp
        //m_player.SetName(packet->name, m_renderManager.GetFont());
        m_player.SetLevel(packet->level, m_renderManager.GetFont());
        m_player.SetExp(packet->exp, m_renderManager.GetFont());
        m_player.Show();
        
        printf("Login success!\n");
        printf("Position: (%d, %d) | Level: %d | EXP: %d | HP: %.0f/%.0f\n",
               m_player.m_x, m_player.m_y, m_player.level, 
               m_player.exp, m_player.hp, m_player.maxHp);
        break;
    }
    
    case SC_PACKET_LOGIN_FAIL:
    {
        m_gameStarted = false;
        std::cout << "Login failed" << std::endl;
        break;
    }
    
    case SC_PACKET_PUT_OBJECT:
    {
        sc_packet_put_object* packet = reinterpret_cast<sc_packet_put_object*>(ptr);
        int id = packet->id;
        
        if (id == m_myId)
        {
            m_player.Move(packet->x, packet->y);
            m_leftX = packet->x - (SCREEN_WIDTH / 2);
            m_topY = packet->y - (SCREEN_HEIGHT / 2);
            m_player.Show();
        }
        else
        {
            // Create NPC or other player
            if (id < NPC_ID_START)
            {
                // Other player
                m_npcs[id] = GameObject(m_renderManager.GetPlayerTexture(), 0, 0, SPRITE_SIZE, SPRITE_SIZE);
                m_npcs[id].SetMoveTexture(m_renderManager.GetPlayerTexture());
            }
            else
            {
                // NPC - Different sprite based on type
                int spriteY = 0;
                if (packet->npcCharacterType == NPC_STAY && packet->npcMoveType == NPC_HOLD)
                    spriteY = 0;
                else if (packet->npcCharacterType == NPC_STAY && packet->npcMoveType == NPC_RANDOM_MOVE)
                    spriteY = 1;
                else if (packet->npcCharacterType == NPC_FIGHT && packet->npcMoveType == NPC_HOLD)
                    spriteY = 2;
                else if (packet->npcCharacterType == NPC_FIGHT && packet->npcMoveType == NPC_RANDOM_MOVE)
                    spriteY = 3;
                else
                    spriteY = 4;
                
                m_npcs[id] = GameObject(m_renderManager.GetPacmanTexture(), 
                                       0, SPRITE_SIZE * spriteY, SPRITE_SIZE, SPRITE_SIZE);
            }
            
            m_npcs[id].m_id = id;
            m_npcs[id].npcCharacterType = packet->npcCharacterType;
            m_npcs[id].npcMoveType = packet->npcMoveType;
            m_npcs[id].SetName(packet->name, m_renderManager.GetFont());
            m_npcs[id].Move(packet->x, packet->y);
            m_npcs[id].Show();
        }
        break;
    }
    
    case SC_PACKET_MOVE:
    {
        sc_packet_move* packet = reinterpret_cast<sc_packet_move*>(ptr);
        int otherId = packet->id;
        
        if (otherId == m_myId)
        {
            m_player.Move(packet->x, packet->y);
            m_leftX = packet->x - (SCREEN_WIDTH / 2);
            m_topY = packet->y - (SCREEN_HEIGHT / 2);
            m_player.Show();
        }
        else
        {
            if (m_npcs.count(otherId) != 0)
            {
                m_npcs[otherId].Move(packet->x, packet->y);
                m_npcs[otherId].direction = packet->direction;
                m_npcs[otherId].Show();
            }
        }
        break;
    }
    
    case SC_PACKET_REMOVE_OBJECT:
    {
        sc_packet_remove_object* packet = reinterpret_cast<sc_packet_remove_object*>(ptr);
        int otherId = packet->id;
        bool isDie = packet->isDie;
        
        if (otherId == m_myId)
        {
            m_player.Hide();
        }
        else
        {
            if (m_npcs.count(otherId) != 0)
            {
                m_npcs[otherId].Hide();
                if (isDie)
                    m_npcs[otherId].hp = -1;
            }
        }
        break;
    }
    
    case SC_PACKET_CHAT:
    {
        sc_packet_chat* packet = reinterpret_cast<sc_packet_chat*>(ptr);
        int oId = packet->id;
        int chatType = packet->chatType;
        
        if (chatType == 0) // Normal chat
        {
            if (m_npcs.count(oId) != 0)
            {
                m_npcs[oId].AddChat(packet->mess, m_renderManager.GetFont());
            }
        }
        else if (chatType == 1) // World chat
        {
            m_renderManager.DrawWorldChat(packet->mess, 0);
        }
        else if (chatType == 2) // Respawn message
        {
            m_renderManager.DrawRespawnText(packet->mess);
        }
        break;
    }
    
    case SC_PACKET_STATUS_CHANGE:
    {
        sc_packet_status_change* packet = reinterpret_cast<sc_packet_status_change*>(ptr);
        int id = packet->id;
        
        if (id == m_myId)
        {
            if (m_player.hp > packet->hp)
                m_player.damageFlag = true;
            
            m_player.hp = static_cast<float>(packet->hp);
            m_player.maxHp = static_cast<float>(packet->maxhp);
            m_player.level = packet->level;
            m_player.exp = packet->exp;
            
            m_player.SetLevel(packet->level, m_renderManager.GetFont());
            m_player.SetExp(packet->exp, m_renderManager.GetFont());
            m_player.Show();
        }
        else
        {
            if (m_npcs.count(id) != 0)
            {
                if (id < NPC_ID_START && m_npcs[id].hp > packet->hp)
                {
                    m_npcs[id].damageFlag = true;
                }
                
                m_npcs[id].hp = static_cast<float>(packet->hp);
                m_npcs[id].maxHp = static_cast<float>(packet->maxhp);
                m_npcs[id].level = packet->level;
                m_npcs[id].exp = packet->exp;
                
                m_npcs[id].SetLevel(packet->level, m_renderManager.GetFont());
                m_npcs[id].SetHp(static_cast<float>(packet->hp), static_cast<float>(packet->maxhp));
                m_npcs[id].Show();
            }
        }
        break;
    }
    
    case SC_PACKET_TELEPORT:
    {
        std::cout << "SC_PACKET_TELEPORT" << std::endl;
        sc_packet_teleport* packet = reinterpret_cast<sc_packet_teleport*>(ptr);
        
        m_player.Move(packet->x, packet->y);
        m_leftX = packet->x - (SCREEN_WIDTH / 2);
        m_topY = packet->y - (SCREEN_HEIGHT / 2);
        m_player.Show();
        break;
    }
    
    default:
        printf("Unknown packet type: %d\n", ptr[1]);
        break;
    }
}
