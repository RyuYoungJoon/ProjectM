#include "pch.h"
#include "PacketHandler.h"
#include "Client.h"
#include "Sender.h"
#include "NPCManager.h"
#include "TimerManager.h"

PacketHandler::PacketHandler(
    std::array<Client, GameConfig::MAX_USER + GameConfig::MAX_NPC>& clients,
    Sender& sender,
    NPCManager& npcManager,
    TimerManager& timerManager,
    MAP (&gameMap)[GameConfig::WORLD_HEIGHT][GameConfig::WORLD_WIDTH]
)
    : m_clients(clients)
    , m_sender(sender)
    , m_npcManager(npcManager)
    , m_timerManager(timerManager)
    , m_gameMap(gameMap)
    , m_acceptedPlayerCount(0)
{
}

void PacketHandler::ProcessPacket(int clientId, char* packet) {
    unsigned char packetType = packet[1];

    switch (packetType) {
    case CS_PACKET_LOGIN:
        HandleLogin(clientId, packet);
        break;
    case CS_PACKET_MOVE:
        HandleMove(clientId, packet);
        break;
    case CS_PACKET_ATTACK:
        HandleAttack(clientId, packet);
        break;
    case CS_PACKET_CHAT:
        HandleChat(clientId, packet);
        break;
    case CS_PACKET_TELEPORT:
        HandleTeleport(clientId, packet);
        break;
    case CS_PACKET_LOGOUT:
        HandleLogout(clientId, packet);
        break;
    }
}

void PacketHandler::HandleLogin(int clientId, char* packet) {
    cs_packet_login* loginPacket = reinterpret_cast<cs_packet_login*>(packet);
    AccessGame(clientId, loginPacket->userID);
}

void PacketHandler::HandleMove(int clientId, char* packet) {
    cs_packet_move* movePacket = reinterpret_cast<cs_packet_move*>(packet);
    m_clients[clientId].direction = movePacket->direction;
    DoMove(clientId, movePacket->direction, movePacket->move_time);
}

void PacketHandler::HandleAttack(int clientId, char* packet) {
    using namespace GameConfig;
    
    Client& attacker = m_clients[clientId];
    
    if (!attacker.can_attack) {
        return;
    }

    std::unordered_set<int> targetList = attacker.viewlist;
    std::unordered_set<int> eraseList;

    for (const auto& targetId : targetList) {
        if (!m_npcManager.IsNPC(targetId)) continue;
        if (!IsAttackable(clientId, targetId)) continue;

        Client& target = m_clients[targetId];
        
        std::lock_guard<std::mutex> lock(target.state_lock);
        AddHp(targetId, attacker.level * -10);

        m_sender.send_status_change(attacker._socket, targetId,
            targetId, target.hp, target.maxhp, target.level, target.exp);

        if (target.hp <= 0) {
            target._state = STATUS::ST_FREE;
            target._is_active = false;
            
            m_sender.send_remove_object(attacker._socket, clientId, targetId, true);
            AddExp(clientId, target.level * target.level * 2);

            if (attacker.exp >= Experience::GetRequiredExp(attacker.level)) {
                AddLevel(clientId, 1);
                attacker.maxhp = Combat::BASE_HP + (Combat::HP_PER_LEVEL * (attacker.level - 1));

                char mess[100];
                sprintf_s(mess, "%s -> Level UP / Level : %d", attacker.name, attacker.level);
                for (int i = 0; i < m_acceptedPlayerCount; ++i) {
                    m_sender.send_chat_packet(m_clients[i]._socket, 1, i, mess);
                }
            }

            m_sender.send_status_change(attacker._socket, clientId,
                clientId, attacker.hp, attacker.maxhp, attacker.level, attacker.exp);

            eraseList.insert(targetId);
        }
    }

    {
        std::lock_guard<std::mutex> lock(attacker.state_lock);
        attacker.can_attack = false;
    }

    for (const auto& targetId : eraseList) {
        std::lock_guard<std::mutex> vlock(attacker.vl);
        attacker.viewlist.erase(targetId);

        std::lock_guard<std::mutex> tlock(m_clients[targetId].vl);
        m_clients[targetId].viewlist.clear();
    }

    m_timerManager.AddTimer(clientId, EVENT_ATTACK, 
        std::chrono::system_clock::now() + std::chrono::seconds(Combat::ATTACK_COOLDOWN_SEC));
}

void PacketHandler::HandleChat(int clientId, char* packet) {
    cs_packet_chat* chatPacket = reinterpret_cast<cs_packet_chat*>(packet);
    
}

void PacketHandler::HandleTeleport(int clientId, char* packet) {
    std::cout << "CS_PACKET_TELEPORT" << std::endl;
    
    m_clients[clientId].x = 500;
    m_clients[clientId].y = 500;

    m_sender.send_teleport_packet(m_clients[clientId]._socket,
        m_clients[clientId].x, m_clients[clientId].y);
}

void PacketHandler::HandleLogout(int clientId, char* packet) {
    
}

void PacketHandler::DoMove(int clientId, char direction, int moveTime) {
    using namespace GameConfig;
    
    Client& client = m_clients[clientId];
    client.last_move_time = moveTime;
    
    int x = client.x;
    int y = client.y;

    switch (direction) {
    case D_UP:
        if (y > 0 && m_gameMap[y - 1][x].type == eBLANK) {
            y--;
            client.direction = D_UP;
        }
        break;
    case D_DOWN:
        if (y < (WORLD_HEIGHT - 1) && m_gameMap[y + 1][x].type == eBLANK) {
            y++;
            client.direction = D_DOWN;
        }
        break;
    case D_LEFT:
        if (x > 0 && m_gameMap[y][x - 1].type == eBLANK) {
            x--;
            client.direction = D_LEFT;
        }
        break;
    case D_RIGHT:
        if (x < (WORLD_WIDTH - 1) && m_gameMap[y][x + 1].type == eBLANK) {
            x++;
            client.direction = D_RIGHT;
        }
        break;
    default:
        std::cout << "Invalid move in Client " << clientId << std::endl;
        return;
    }

    client.x = x;
    client.y = y;

    std::unordered_set<int> nearList;
    for (auto& other : m_clients) {
        if (other._id == clientId) continue;
        if (other._state != STATUS::ST_INGAME) continue;
        if (!m_npcManager.IsNear(clientId, other._id)) continue;

        if (m_npcManager.IsNPC(other._id)) {
            m_npcManager.WakeUpNPC(other._id);
        }
        nearList.insert(other._id);
    }

    m_sender.send_move_packet(client._socket, clientId, clientId,
        client.x, client.y, client.last_move_time, client.direction);

    std::unordered_set<int> myViewList;
    {
        std::lock_guard<std::mutex> lock(client.vl);
        myViewList = client.viewlist;
    }

    for (auto otherId : nearList) {
        if (myViewList.count(otherId) == 0) {
            {
                std::lock_guard<std::mutex> lock(client.vl);
                client.viewlist.insert(otherId);
            }

            m_sender.send_put_object(client._socket, clientId,
                otherId, m_clients[otherId].x, m_clients[otherId].y, m_clients[otherId].name,
                m_clients[otherId].npcCharacterType, m_clients[otherId].npcMoveType);
            m_sender.send_status_change(client._socket, clientId,
                otherId, m_clients[otherId].hp, m_clients[otherId].maxhp,
                m_clients[otherId].level, m_clients[otherId].exp);

            if (m_npcManager.IsNPC(otherId)) continue;

            Client& other = m_clients[otherId];
            std::lock_guard<std::mutex> olock(other.vl);
            if (other.viewlist.count(clientId) == 0) {
                other.viewlist.insert(clientId);
                m_sender.send_put_object(other._socket, otherId,
                    clientId, client.x, client.y, client.name,
                    client.npcCharacterType, client.npcMoveType);
                m_sender.send_status_change(other._socket, otherId,
                    clientId, client.hp, client.maxhp, client.level, client.exp);
            } else {
                m_sender.send_move_packet(other._socket, otherId,
                    clientId, client.x, client.y, client.last_move_time, client.direction);
            }
        } else {
            if (m_npcManager.IsNPC(otherId)) continue;

            Client& other = m_clients[otherId];
            std::lock_guard<std::mutex> olock(other.vl);
            if (other.viewlist.count(clientId) > 0) {
                m_sender.send_move_packet(other._socket, otherId,
                    clientId, client.x, client.y, client.last_move_time, client.direction);
            } else {
                other.viewlist.insert(clientId);
                m_sender.send_put_object(other._socket, otherId,
                    clientId, client.x, client.y, client.name,
                    client.npcCharacterType, client.npcMoveType);
                m_sender.send_status_change(other._socket, otherId,
                    clientId, client.hp, client.maxhp, client.level, client.exp);
            }
        }
    }

    // 시야에서 벗어난 객체 처리
    for (auto otherId : myViewList) {
        if (nearList.count(otherId) == 0) {
            {
                std::lock_guard<std::mutex> lock(client.vl);
                client.viewlist.erase(otherId);
            }
            m_sender.send_remove_object(client._socket, clientId, otherId, false);

            if (m_npcManager.IsNPC(otherId)) continue;

            Client& other = m_clients[otherId];
            std::lock_guard<std::mutex> olock(other.vl);
            if (other.viewlist.count(clientId) > 0) {
                other.viewlist.erase(clientId);
                m_sender.send_remove_object(other._socket, otherId, clientId, false);
            }
        }
    }
}

void PacketHandler::AccessGame(int userId, const char* name) {
    using namespace GameConfig;
    
    Client& client = m_clients[userId];
    
    std::lock_guard<std::mutex> lock(client.cl);
    strcpy_s(client.name, name);
    
    m_sender.send_login_ok_packet(client._socket, userId,
        client.x, client.y, client.level, client.hp, client.exp);

    if (client.hp < Combat::BASE_HP) {
        m_timerManager.AddTimer(userId, EVENT_HEAL, 
            std::chrono::system_clock::now() + std::chrono::seconds(Combat::HEAL_INTERVAL_SEC));
    }

    client._state = STATUS::ST_INGAME;

    for (auto& other : m_clients) {
        int otherId = other._id;
        if (userId == otherId) continue;

        if (m_npcManager.IsNear(userId, otherId)) {
            if (other._state == STATUS::ST_FREE) {
                other._state = STATUS::ST_INGAME;
            }

            if (other._state == STATUS::ST_INGAME) {
                m_sender.send_put_object(client._socket, userId,
                    otherId, other.x, other.y, other.name,
                    other.npcCharacterType, other.npcMoveType);
                m_sender.send_status_change(client._socket, userId,
                    otherId, other.hp, other.maxhp, other.level, other.exp);

                if (m_npcManager.IsPlayer(otherId)) {
                    m_sender.send_put_object(other._socket, otherId,
                        userId, client.x, client.y, client.name,
                        client.npcCharacterType, client.npcMoveType);
                    m_sender.send_status_change(other._socket, otherId,
                        userId, client.hp, client.maxhp, client.level, client.exp);
                } else if (other.npcCharacterType == GameConfig::NPC_FIGHT) {
                    m_npcManager.MoveNPC(otherId);
                }
            }
        }
    }
}

bool PacketHandler::IsAttackable(int attackerId, int targetId) const {
    const Client& attacker = m_clients[attackerId];
    const Client& target = m_clients[targetId];

    if (attacker.x == target.x) {
        return abs(attacker.y - target.y) <= 1;
    } else if (attacker.y == target.y) {
        return abs(attacker.x - target.x) <= 1;
    }
    return false;
}

void PacketHandler::AddExp(int clientId, short value) {
    m_clients[clientId].exp += value;
    std::cout << "add_exp : " << m_clients[clientId].exp << std::endl;

    char mess[100];
    sprintf_s(mess, "%s -> ADD EXP +%d", m_clients[clientId].name, value);
    m_sender.send_chat_packet(m_clients[clientId]._socket, clientId, clientId, mess);
}

void PacketHandler::AddLevel(int clientId, short value) {
    m_clients[clientId].level += value;
}

void PacketHandler::AddHp(int clientId, short value) {
    m_clients[clientId].hp += value;
}
