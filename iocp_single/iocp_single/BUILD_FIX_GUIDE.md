# 빌드 에러 수정 가이드

## 주요 문제점

1. **pch.h 파일이 잠김** - Visual Studio를 닫고 수정 필요
2. **send_put_object 함수 인수 불일치** - object_type 누락
3. **헤더 include 순서** 문제

## 수정 방법

### 1단계: Visual Studio 완전 종료
- 모든 Visual Studio 인스턴스 닫기
- 작업 관리자에서 devenv.exe 프로세스 종료 확인

### 2단계: pch.h 수정

`pch.h`의 마지막 부분을 다음과 같이 수정:

```cpp
#include "GameConfig.h"
#include "Protocol.h"
#include "enum.h"
#include "event.h"
#include "error.h"
#include "Client.h"
#include "Sender.h"
#include "DB.h"

// Manager 클래스는 forward declaration만
// 실제 include는 cpp 파일에서만!

#endif
```

### 3단계: send_put_object 호출 수정

**모든 send_put_object 호출 시 object_type 추가:**

변경 전:
```cpp
m_sender.send_put_object(socket, c_id, target, x, y, name, 
    npcCharacterType, npcMoveType);
```

변경 후:
```cpp
m_sender.send_put_object(socket, c_id, target, x, y, name, 
    2, npcCharacterType, npcMoveType);  // 2 = NPC
```

플레이어인 경우:
```cpp
m_sender.send_put_object(socket, c_id, target, x, y, name, 
    1, 0, 0);  // 1 = Player
```

### 4단계: 수정할 파일 목록

#### NPCManager.cpp (177번째 줄 근처)
```cpp
m_sender.send_put_object(m_clients[pl]._socket, pl,
    npcId, npc.x, npc.y, npc.name,
    2, npc.npcCharacterType, npc.npcMoveType);  // 2 추가
```

#### PacketHandler.cpp (여러 곳)
모든 send_put_object 호출에 object_type 추가:
- 플레이어: 1
- NPC: 2

예시:
```cpp
// NPC인 경우
m_sender.send_put_object(client._socket, clientId,
    otherId, other.x, other.y, other.name,
    2, other.npcCharacterType, other.npcMoveType);

// 플레이어인 경우  
m_sender.send_put_object(other._socket, otherId,
    clientId, client.x, client.y, client.name,
    1, 0, 0);
```

### 5단계: GameServer.cpp 수정

#### 100번째 줄 근처 (range-based for 에러)
```cpp
// 변경 전
for (auto& thread : m_workerThreads)

// 변경 후
for (size_t i = 0; i < m_workerThreads.size(); ++i)
    if (m_workerThreads[i]->joinable())
        m_workerThreads[i]->join();
```

#### 114번째 줄 근처 (range-based for 에러)
```cpp
// 변경 전
for (auto& client : m_clients)

// 변경 후
for (size_t i = 0; i < m_clients.size(); ++i)
    if (m_clients[i]._state == STATUS::ST_INGAME)
        DisconnectClient(m_clients[i]._id);
```

### 6단계: Sender.h 확인

Sender.h에 다음 오버로드가 있는지 확인:

```cpp
void send_put_object(SOCKET s, int c_id, int target,
    short x, short y, char* name, char object_type,
    char npcCharactherType, char npcMoveType);
```

없으면 추가 필요!

## 빠른 해결 방법

가장 빠른 방법은:
1. Visual Studio 완전 종료
2. 기존 Server.cpp를 Server.cpp.old로 백업
3. 기존 코드 그대로 사용하되, 상수만 GameConfig 사용하도록 수정
4. 점진적 리팩토링 진행

## 또는...

현재 작성한 모든 새 파일을 프로젝트에서 제거하고,
기존 Server.cpp만 사용하면서 작은 부분부터 리팩토링하는 것도 방법입니다.

원하시는 방향으로 진행하시겠어요?
