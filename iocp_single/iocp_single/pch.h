#pragma once
#define _CRT_SECURE_NO_WARNINGS

#ifndef PCH_H
#define PCH_H

// 표준 라이브러리
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <array>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include <queue>
#include <unordered_map>
#include <sqlext.h>  
#include <chrono>

// Lua
extern "C" {
#include "include\lua.h"
#include "include\lauxlib.h"
#include "include\lualib.h"
}
#pragma comment (lib, "lua54.lib")

using namespace std;
using namespace chrono;

// 링크 라이브러리
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")
#pragma comment(lib,"odbc32.lib")

// 레거시 정의
#define NAME_LEN 50  
#define PHONE_LEN 60

// 프로젝트 헤더 (의존성 순서대로)
#include "GameConfig.h"
#include "Protocol.h"
#include "enum.h"          // Client.h보다 먼저!
#include "event.h"
#include "error.h"
#include "Client.h"        // enum.h 이후
#include "Sender.h"
#include "DB.h"

// Manager 헤더들은 forward declaration만 (순환 의존성 방지)
// 실제 include는 각 cpp 파일에서!

#endif
