#include "pch.h"
#include "GameServer.h"
#include "Client.h"
#include "Sender.h"
#include "PacketHandler.h"
#include "NPCManager.h"
#include "TimerManager.h"
#include "error.h"

GameServer::GameServer()
    : m_listenSocket(INVALID_SOCKET)
    , m_hIOCP(nullptr)
    , m_acceptedPlayerCount(0)
    , m_running(false)
{
    // Client ID
    for (int i = 0; i < GameConfig::MAX_USER; ++i) {
        m_clients[i]._id = i;
    }
}

GameServer::~GameServer() {
    Shutdown();
}

bool GameServer::Initialize() {
    // Winsock 
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GameConfig::SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        return false;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return false;
    }

    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (m_hIOCP == nullptr) {
        std::cerr << "IOCP creation failed" << std::endl;
        return false;
    }

    CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_hIOCP, 0, 0);

    m_sender = std::make_unique<Sender>();
    m_timerManager = std::make_unique<TimerManager>();
    m_npcManager = std::make_unique<NPCManager>(m_clients, *m_sender, *m_timerManager, m_gameMap);
    m_packetHandler = std::make_unique<PacketHandler>(m_clients, *m_sender, *m_npcManager, *m_timerManager, m_gameMap);

    InitializeMap();

    m_npcManager->InitializeNPCs();

    m_timerManager->Start(m_hIOCP);

    SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    Exp_Over* acceptEx = new Exp_Over;
    ZeroMemory(&acceptEx->_wsa_over, sizeof(acceptEx->_wsa_over));
    acceptEx->_comp_op = COMP_OP::OP_ACCEPT;
    *(reinterpret_cast<SOCKET*>(acceptEx->_net_buf)) = clientSocket;

    char acceptBuf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
    AcceptEx(m_listenSocket, clientSocket, acceptBuf, 0, 
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &acceptEx->_wsa_over);

    std::cout << "Server initialized successfully" << std::endl;
    return true;
}

void GameServer::Run() {
    m_running = true;

    for (int i = 0; i < GameConfig::WORKER_THREAD_COUNT; ++i) {
        m_workerThreads.emplace_back(
            std::make_unique<std::thread>(&GameServer::WorkerThread, this)
        );
    }

    std::cout << "Server is running..." << std::endl;

    for (auto& thread : m_workerThreads) {
        if (thread->joinable()) {
            thread->join();
        }
    }
}

void GameServer::Shutdown() {
    m_running = false;

    if (m_timerManager) {
        m_timerManager->Stop();
    }

    for (auto& client : m_clients) {
        if (client._state == STATUS::ST_INGAME) {
            DisconnectClient(client._id);
        }
    }

    // 소켓 정리
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    WSACleanup();
    std::cout << "Server shutdown complete" << std::endl;
}

int GameServer::GetNewClientId() {
    for (int i = 0; i < GameConfig::MAX_USER; ++i) {
        std::lock_guard<std::mutex> lock(m_clients[i].state_lock);
        if (m_clients[i]._state == STATUS::ST_FREE) {
            m_clients[i]._state = STATUS::ST_ACCEPT;
            return i;
        }
    }
    std::cout << "Maximum number of clients overflow!" << std::endl;
    return -1;
}

void GameServer::DisconnectClient(int clientId) {
    Client& client = m_clients[clientId];
    
    std::lock_guard<std::mutex> vlock(client.vl);
    std::unordered_set<int> myViewlist = client.viewlist;
    
    for (auto otherId : myViewlist) {
        Client& other = m_clients[otherId];
        if (IsNPC(otherId)) continue;
        if (other._state != STATUS::ST_INGAME) continue;

        std::lock_guard<std::mutex> olock(other.vl);
        if (other.viewlist.count(clientId) > 0) {
            other.viewlist.erase(clientId);
            m_sender->send_remove_object(other._socket, otherId, clientId, false);
        }
    }

    std::lock_guard<std::mutex> slock(client.state_lock);
    closesocket(client._socket);
    client._state = STATUS::ST_FREE;
}

bool GameServer::IsNear(int id1, int id2) const {
    if (GameConfig::VIEW_RANGE < abs(m_clients[id1].x - m_clients[id2].x)) return false;
    if (GameConfig::VIEW_RANGE < abs(m_clients[id1].y - m_clients[id2].y)) return false;
    return true;
}

bool GameServer::IsNPC(int id) const {
    return (id >= GameConfig::NPC_ID_START) && (id <= GameConfig::NPC_ID_END);
}

bool GameServer::IsPlayer(int id) const {
    return (id >= 0) && (id < GameConfig::MAX_USER);
}

void GameServer::PlayerHeal(int clientId) {
    using namespace GameConfig;
    using namespace GameConfig::Combat;

    Client& client = m_clients[clientId];
    int maxHp = BASE_HP + (HP_PER_LEVEL * (client.level - 1));

    if (client.hp < maxHp) {
        std::lock_guard<std::mutex> lock(client.state_lock);
        client.hp += static_cast<short>(maxHp * HEAL_RATE);
        if (client.hp > maxHp) {
            client.hp = maxHp;
        }

        m_sender->send_status_change(client._socket, clientId,
            clientId, client.hp, client.maxhp, client.level, client.exp);

        char mess[100];
        sprintf_s(mess, "%s -> ADD HP +%d", client.name, static_cast<int>(maxHp * HEAL_RATE));
        m_sender->send_chat_packet(client._socket, clientId, clientId, mess);
    }

    m_timerManager->AddTimer(clientId, EVENT_HEAL, 
        std::chrono::system_clock::now() + std::chrono::seconds(HEAL_INTERVAL_SEC));
}

void GameServer::InitializeMap() {
    FILE* fp = nullptr;
    fopen_s(&fp, "mapData.txt", "rb");
    if (!fp) {
        std::cerr << "Failed to open mapData.txt" << std::endl;
        return;
    }

    char data;
    int count = 0;

    while (fscanf_s(fp, "%c", &data, 1) != EOF) {
        switch (data) {
        case '0':
            m_gameMap[count / GameConfig::WORLD_WIDTH][count % GameConfig::WORLD_WIDTH].type = eBLANK;
            count++;
            break;
        case '3':
            m_gameMap[count / GameConfig::WORLD_WIDTH][count % GameConfig::WORLD_WIDTH].type = eBLOCKED;
            count++;
            break;
        }
    }

    fclose(fp);
    std::cout << "Map initialized" << std::endl;
}

void GameServer::WorkerThread() {
    while (m_running) {
        DWORD numBytes = 0;
        ULONG_PTR key = 0;
        WSAOVERLAPPED* overlapped = nullptr;

        BOOL ret = GetQueuedCompletionStatus(m_hIOCP, &numBytes, &key, &overlapped, INFINITE);
        
        int clientId = static_cast<int>(key);
        Exp_Over* expOver = reinterpret_cast<Exp_Over*>(overlapped);

        

        switch (expOver->_comp_op) {
        case COMP_OP::OP_RECV:

            if (!ret || numBytes == 0) {
                if (expOver->_comp_op != COMP_OP::OP_ACCEPT && expOver->_comp_op != COMP_OP::OP_SEND) {
                    DisconnectClient(clientId);
                }
                if (expOver->_comp_op == COMP_OP::OP_SEND) {
                    delete expOver;
                }
                continue;
            }

            HandleRecv(clientId, expOver, numBytes);
            break;
        case COMP_OP::OP_SEND:
            HandleSend(clientId, expOver, numBytes);
            break;
        case COMP_OP::OP_ACCEPT:
            HandleAccept(expOver);
            break;
        case COMP_OP::OP_NPC_MOVE:
            HandleNPCMove(clientId, expOver);
            break;
        case COMP_OP::OP_ATTACK:
            HandleAttack(clientId, expOver);
            break;
        case COMP_OP::OP_REVIVE:
            HandleRevive(clientId, expOver);
            break;
        case COMP_OP::OP_HEAL:
            HandleHeal(clientId, expOver);
            break;
        case COMP_OP::OP_PLAYER_MOVE:
        {
            lua_State* L = m_clients[clientId].L;
            lua_getglobal(L, "event_player_attack");
            lua_pushnumber(L, clientId);
            lua_pushnumber(L, m_clients[clientId].x);
            lua_pushnumber(L, m_clients[clientId].y);
            lua_pcall(L, 3, 0, 0);
            delete expOver;
            break;
        }
        }
    }
}

void GameServer::HandleRecv(int clientId, Exp_Over* expOver, DWORD numBytes) {
    Client& client = m_clients[clientId];
    int remainData = numBytes + client._prev_size;
    char* packetStart = expOver->_net_buf;
    int packetSize = packetStart[0];

    while (packetSize <= remainData) {
        m_packetHandler->ProcessPacket(clientId, packetStart);
        remainData -= packetSize;
        packetStart += packetSize;
        if (remainData > 0) {
            packetSize = packetStart[0];
        } else {
            break;
        }
    }

    if (remainData > 0) {
        client._prev_size = remainData;
        memcpy(expOver->_net_buf, packetStart, remainData);
    } else {
        client._prev_size = 0;
    }

    // 다음 Recv 요청
    ZeroMemory(&client._recv_over._wsa_over, sizeof(client._recv_over._wsa_over));
    DWORD recvFlag = 0;
    WSARecv(client._socket, &client._recv_over._wsa_buf, 1, 0, &recvFlag, 
        &client._recv_over._wsa_over, NULL);
}

void GameServer::HandleSend(int clientId, Exp_Over* expOver, DWORD numBytes) {
    if (numBytes != expOver->_wsa_buf.len) {
        DisconnectClient(clientId);
    }
    delete expOver;
}

void GameServer::HandleAccept(Exp_Over* expOver) {
    std::cout << "Accept completed" << std::endl;

    SOCKET clientSocket = *(reinterpret_cast<SOCKET*>(expOver->_net_buf));
    int newId = GetNewClientId();

    if (newId == -1) {
        std::cout << "Maximum user overflow. Accept aborted" << std::endl;
        closesocket(clientSocket);
    } else {
        Client& client = m_clients[newId];
        CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), m_hIOCP, newId, 0);

        client.x = rand() % GameConfig::WORLD_WIDTH;
        client.y = rand() % GameConfig::WORLD_HEIGHT;
        client._id = newId;
        client.hp = 100;
        client.exp = 0;
        client.level = 1;
        client.can_attack = true;
        client.maxhp = GameConfig::Combat::BASE_HP;
        client._socket = clientSocket;

        m_acceptedPlayerCount++;

        client._recv_over._comp_op = COMP_OP::OP_RECV;
        client._recv_over._wsa_buf.buf = reinterpret_cast<char*>(client._recv_over._net_buf);
        client._recv_over._wsa_buf.len = sizeof(client._recv_over._net_buf);
        ZeroMemory(&client._recv_over._wsa_over, sizeof(client._recv_over._wsa_over));

        DWORD recvFlag = 0;
        int ret = WSARecv(client._socket, &client._recv_over._wsa_buf, 1, 0, 
            &recvFlag, &client._recv_over._wsa_over, NULL);
        
        if (ret == SOCKET_ERROR) {
            int errorNum = WSAGetLastError();
            if (errorNum != ERROR_IO_PENDING) {
                std::cerr << "WSARecv error: " << errorNum << std::endl;
            }
        }
    }

    ZeroMemory(&expOver->_wsa_over, sizeof(expOver->_wsa_over));
    clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    *(reinterpret_cast<SOCKET*>(expOver->_net_buf)) = clientSocket;
    
    AcceptEx(m_listenSocket, clientSocket, expOver->_net_buf + 8, 0,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &expOver->_wsa_over);
}

void GameServer::HandleNPCMove(int clientId, Exp_Over* expOver) {
    m_npcManager->MoveNPC(clientId);
    delete expOver;
}

void GameServer::HandleAttack(int clientId, Exp_Over* expOver) {
    std::lock_guard<std::mutex> lock(m_clients[clientId].state_lock);
    m_clients[clientId].can_attack = true;
    delete expOver;
}

void GameServer::HandleRevive(int clientId, Exp_Over* expOver) {
    std::cout << "REVIVE" << std::endl;
    Client& client = m_clients[clientId];
    
    std::lock_guard<std::mutex> lock(client.state_lock);
    client._recv_over._comp_op = COMP_OP::OP_RECV;
    client.hp = 10;
    client.level = 1;
    client.overlap = false;
    client.move_count = 0;

    m_npcManager->MoveNPC(clientId);
    delete expOver;
}

void GameServer::HandleHeal(int clientId, Exp_Over* expOver) {
    PlayerHeal(clientId);
    delete expOver;
}
