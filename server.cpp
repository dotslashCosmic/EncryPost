#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT 53100

void startServer() {
    WSADATA wsaData;
    SOCKET server_sock, client_sock; // Ensure client_sock is defined
    struct sockaddr_in server_address, client_address;
    int addr_len = sizeof(client_address);
    char buffer[1024] = {0};

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return;
    }

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation failed");
        WSACleanup();
        return;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    server_address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        closesocket(server_sock);
        WSACleanup();
        return;
    }

    // Listen for incoming connections
    listen(server_sock, 3);
    std::cout << "Server listening on port " << PORT << "...\n";

    // Accept a connection
    if ((client_sock = accept(server_sock, (struct sockaddr *)&client_address, &addr_len)) < 0) {
        perror("Accept failed");
        closesocket(server_sock);
        WSACleanup();
        return;
    }

    // Handle incoming messages
    while (true) {
        int valread = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            std::cerr << "Connection closed or error receiving message" << std::endl;
            break;
        }
        
        buffer[valread] = '\0'; // Null-terminate the received string

        // Check if the message is a join message and do not echo it back
        std::string receivedMessage(buffer);
        if (receivedMessage.find("Joining group:") == std::string::npos) {
            std::cout << "Received message: " << buffer << std::endl;
            // Echo the message back to the client
            send(client_sock, buffer, valread, 0);
        }
    }

    // Cleanup
    closesocket(client_sock);
    closesocket(server_sock);
    WSACleanup();
}

int main() {
    startServer();
    return 0;
}
