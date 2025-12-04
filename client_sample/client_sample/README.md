# 2D Pixel RPG Client - 리팩토링 완료

## 📋 개요
기존의 단일 파일 `client.cpp`를 객체지향 설계로 리팩토링하고, 일반적인 2D 픽셀 RPG에 적합한 수치로 변경했습니다.

## 🎯 주요 변경사항

### 1. 일반적인 2D RPG 수치로 변경
| 항목 | 기존 값 | 변경 후 | 이유 |
|------|---------|---------|------|
| 타일 크기 | 65x65 | 32x32 | 일반적인 픽셀 RPG 타일 크기 |
| 화면 타일 수 (가로) | 20 | 25 | 더 넓은 시야 |
| 화면 타일 수 (세로) | 20 | 19 | 16:9 비율에 가깝게 |
| 윈도우 크기 | 가변 | 800x608 | 표준 해상도 |
| 플레이어 스케일 | 3.5 | 2.5 | 타일 크기에 맞춤 |
| NPC 스케일 | 2.0 | 2.0 | 유지 |
| FPS | 25 (40ms) | 60 | 부드러운 애니메이션 |

### 2. 코드 구조 개선

#### 파일 구조
```
client_sample/
├── Config.h                      # 게임 설정 상수
├── GameObject.h/cpp              # 게임 오브젝트 클래스
├── MapManager.h/cpp              # 맵 관리
├── NetworkManager.h/cpp          # 네트워크 통신
├── RenderManager.h/cpp           # 렌더링 관리
├── GameManager.h/cpp             # 게임 전체 관리
├── GameManager_PacketHandler.cpp # 패킷 처리 로직
└── main.cpp                      # 진입점
```

#### 클래스 다이어그램
```
GameManager
├── MapManager
│   └── GameObject (타일들)
├── NetworkManager
├── RenderManager
│   ├── Textures
│   └── UI Elements
└── GameObjects
    ├── Player (GameObject)
    └── NPCs (map<int, GameObject>)
```

## 📦 클래스 설명

### Config.h
게임의 모든 상수를 중앙 집중 관리
- 화면 설정
- UI 설정
- 게임 로직 설정
- 캐릭터 설정
- 색상 정의
- 리소스 경로

### GameObject
게임 내 모든 엔티티의 기본 클래스
- 위치, 스탯, 애니메이션 관리
- 렌더링 및 업데이트
- 채팅 메시지 표시

### MapManager
게임 맵 전체 관리
- 맵 데이터 로드
- 타일 렌더링
- 충돌 체크

### NetworkManager
네트워크 통신 전담
- 서버 연결/해제
- 패킷 송수신
- 패킷 전송 함수들

### RenderManager
모든 렌더링 관련 작업
- 텍스처 로드 및 관리
- UI 렌더링
- 폰트 관리

### GameManager
게임 전체 로직 통합 관리
- 게임 루프
- 입력 처리
- 패킷 처리
- 게임 상태 관리

## 🎮 주요 개선 사항

### 1. 가독성
- 단일 파일 2000+ 줄 → 여러 파일로 분리
- 명확한 책임 분리
- 의미있는 네이밍

### 2. 유지보수성
- 모듈화된 구조
- 설정 값 중앙 관리
- 쉬운 기능 추가/수정

### 3. 확장성
- 새로운 게임 오브젝트 추가 용이
- 새로운 패킷 타입 추가 간편
- UI 커스터마이징 쉬움

### 4. 성능
- FPS 25 → 60 향상
- 효율적인 렌더링
- 최적화된 패킷 처리

## 🔧 설정 가능한 값

`Config.h`에서 다음 값들을 쉽게 조정할 수 있습니다:

```cpp
// 타일 크기 변경
constexpr int TILE_SIZE = 32;  // 16, 24, 32, 48 등

// 화면 크기 변경
constexpr int SCREEN_WIDTH = 25;
constexpr int SCREEN_HEIGHT = 19;

// FPS 조정
constexpr int FPS = 60;

// UI 크기 조정
constexpr int HP_BAR_WIDTH = 80;
constexpr int LEVEL_CIRCLE_RADIUS = 15;

// 폰트 크기 조정
constexpr int FONT_SIZE_NORMAL = 18;
```

## 🚀 빌드 및 실행

### 요구사항
- SFML 2.5+
- Visual Studio 2019+
- C++17 이상

### 빌드
1. Visual Studio에서 프로젝트 열기
2. 빌드 구성 선택 (Debug/Release)
3. 빌드 (Ctrl + Shift + B)

### 실행
1. 서버가 실행 중인지 확인
2. 클라이언트 실행
3. ID와 비밀번호 입력
4. 게임 시작!

## 🎨 일반적인 2D RPG 스타일

### 타일 크기
- 32x32: 대부분의 고전 RPG (Final Fantasy, Dragon Quest 스타일)
- 16x16: 레트로 스타일
- 48x48: 좀 더 디테일한 그래픽

### 화면 비율
- 800x600 (4:3): 고전적인 비율
- 800x608 (타일 정렬): 19타일 높이로 깔끔한 정렬

### 캐릭터 스프라이트
- 31x31 원본 스프라이트
- 2.5배 스케일 = 약 77x77 픽셀 (32x32 타일의 2.4배)
- 타일보다 크게 보여서 임팩트 있는 캐릭터 표현

## 📝 추가 개선 가능 사항

1. **사운드 시스템 추가**
   - AudioManager 클래스 생성
   - BGM 및 효과음 재생

2. **인벤토리 시스템**
   - InventoryManager 클래스
   - 아이템 관리

3. **스킬 시스템**
   - SkillManager 클래스
   - 스킬 이펙트 및 쿨타임

4. **미니맵**
   - MinimapRenderer 추가
   - 실시간 맵 표시

5. **파티클 시스템**
   - ParticleManager 추가
   - 이펙트 강화

## 📄 라이선스
원본 코드를 리팩토링한 버전입니다.

## 👨‍💻 작성자
Refactored by AI Assistant for better code quality and maintainability.
