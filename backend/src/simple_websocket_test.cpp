/**
 * Simple WebSocket Server Test
 * Minimal version for debugging
 */

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Simple WebSocket Server Test" << std::endl;
    std::cout << "========================================" << std::endl;

    // Initialize Winsock
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return 1;
    }

    std::cout << "Winsock initialized successfully!" << std::endl;
    std::cout << "Press Ctrl+C to exit..." << std::endl;

    // Simple loop to keep program running
    int counter = 0;
    while (counter < 5) {
        std::cout << "[INFO] Heartbeat #" << (counter + 1) << std::endl;
        Sleep(2000);  // Sleep for 2 seconds
        counter++;
    }

    std::cout << "Test completed successfully!" << std::endl;

    // Cleanup
    WSACleanup();

    return 0;
}
