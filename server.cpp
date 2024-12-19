#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#define MAIN_PORT 53100
#define KNOCK_PORT_1 53101
#define KNOCK_PORT_2 53102
#define KNOCK_PORT_3 53103

std::vector<int> knockSequence = {KNOCK_PORT_1, KNOCK_PORT_2, KNOCK_PORT_3};
std::vector<int> receivedKnocks;

bool isKnockSequenceValid() {
    return receivedKnocks.size() == knockSequence.size() && std::equal(receivedKnocks.begin(), receivedKnocks.end(), knockSequence.begin());
}

void handleClient(SOCKET client_sock) {
    char buffer[1024] = {0};
    while (true) {
        int valread = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            std::cerr << "Connection closed or error receiving message" << std::endl;
            break;
        }
        
        buffer[valread] = '\0'; // Null-terminate the received string
        std::cout << "Received message: " << buffer << std::endl;
        // Echo the message back to the client
        send(client_sock, buffer, valread, 0);
    }
    closesocket(client_sock);
}

void startServer() {
    WSADATA wsaData;
    SOCKET server_sock; 
    struct sockaddr_in server_address, client_address;
    int addr_len = sizeof(client_address);

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
    server_address.sin_addr.s_addr = INADDR_ANY; 
    server_address.sin_port = htons(MAIN_PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        closesocket(server_sock);
        WSACleanup();
        return;
    }

    // Listen for incoming connections
    listen(server_sock, 3);
    std::cout << "Server listening for knocks on ports 53101, 53102, 53103...\n";

    // Accept connections in a loop
    while (true) {
        SOCKET knock_client_sock = accept(server_sock, (struct sockaddr *)&client_address, &addr_len);
        if (knock_client_sock < 0) {
            perror("Accept failed");
            continue; 
        }
        receivedKnocks.push_back(ntohs(client_address.sin_port)); // Store the received knock port
        closesocket(knock_client_sock);

        // Check if the knock sequence is valid
        if (isKnockSequenceValid()) {
            std::cout << "Valid knock sequence received. Opening main port for connections.\n";
            server_address.sin_port = htons(MAIN_PORT);
            listen(server_sock, 3); // Reuse the same socket for main connections

            // Notify the specific mnemonic code server about the joining request
            std::string username = "UserX"; // Replace "UserX" with the actual username
            std::string message = username + " wishes to join the group. Accept? (y/n)";
            send(knock_client_sock, message.c_str(), message.length(), 0);
        }
    }

    // Cleanup
    closesocket(server_sock);
    WSACleanup();
}

int main() {
    startServer();
    return 0;
}