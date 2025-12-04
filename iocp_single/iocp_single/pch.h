#pragma once
#define _CRT_SECURE_NO_WARNINGS

#ifndef PCH_H
#define PCH_H

// Standard Library
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

// Link Libraries
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")
#pragma comment(lib,"odbc32.lib")

// Legacy Definitions
#define NAME_LEN 50  
#define PHONE_LEN 60

// Project Headers (in dependency order)
#include "GameConfig.h"
#include "Protocol.h"
#include "enum.h"          // Before Client.h
#include "event.h"
#include "error.h"
#include "Client.h"        // After enum.h
#include "Sender.h"
#include "DB.h"

// Manager headers: forward declaration only (to prevent circular dependencies)
// Actual includes should be in each cpp file

#endif
