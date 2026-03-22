/**
 * WebSocket Test Server
 * Simple server to test WebSocket functionality
 */

#include "websocket_server.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    std::cout << "========================================" << std::endl;
    std::cout << "  WebSocket Test Server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Starting WebSocket server on port 8088..." << std::endl;

    WebSocketServer server(8088);

    // Set up event handlers
    server.setConnectionHandler([](int client_fd) {
        std::cout << "[INFO] Client connected: " << client_fd << std::endl;
    });

    server.setMessageHandler([](const WSMessage& message) {
        std::cout << "[INFO] Received message: " << message.toJSON() << std::endl;
    });

    server.setDisconnectionHandler([](int client_fd) {
        std::cout << "[INFO] Client disconnected: " << client_fd << std::endl;
    });

    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server started successfully!" << std::endl;
    std::cout << "WebSocket endpoint: ws://localhost:8088/ws" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << "========================================" << std::endl;

    // Send a heartbeat every 30 seconds
    int counter = 0;
    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        counter++;

        std::cout << "[INFO] Server heartbeat #" << counter
                  << " | Active connections: " << server.getConnectionCount() << std::endl;
    }

    std::cout << "Shutting down..." << std::endl;
    server.stop();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
