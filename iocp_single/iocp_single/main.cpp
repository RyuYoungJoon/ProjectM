#include "pch.h"
#include "GameServer.h"

int main() {
    std::wcout.imbue(std::locale("korean"));
    
    GameServer server;
    
    if (!server.Initialize()) {
        std::cerr << "Server initialization failed!" << std::endl;
        return -1;
    }

    server.Run();
    
    return 0;
}
