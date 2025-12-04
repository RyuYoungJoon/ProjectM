#define _CRT_SECURE_NO_WARNINGS
#define SFML_STATIC 1

#include <iostream>
#include "GameManager.h"

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#else
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#endif

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

int main()
{
    std::cout << "======================================" << std::endl;
    std::cout << "   2D Pixel RPG Client - Refactored  " << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;
    
    auto game = std::make_unique<GameManager>();
    
    if (!game->Initialize())
    {
        std::cerr << "Game initialization failed!" << std::endl;
        system("pause");
        return -1;
    }
    
    std::cout << "Game starting..." << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow Keys - Move" << std::endl;
    std::cout << "  Space      - Attack" << std::endl;
    std::cout << "  A          - Teleport" << std::endl;
    std::cout << "  ESC        - Exit" << std::endl;
    std::cout << std::endl;
    
    game->Run();
    
    std::cout << "Game ended. Thank you for playing!" << std::endl;
    return 0;
}
