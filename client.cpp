#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <windows.h>
#include <sstream>
#include <vector>
#include <string>
#include "encryption.h"

#define PORT 53100

SOCKET sock;
HWND hMessageInput, hMessageDisplay, hMnemonicInput, hMnemonicCodeDisplay;

std::string fetchPublicIP() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[1024];
    std::string ip;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "Error fetching IP: WSAStartup failed.";
    }

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    server.sin_addr.s_addr = inet_addr("54.240.196.61"); // IP of checkip.amazonaws.com

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        closesocket(sock);
        WSACleanup();
        return "Error fetching IP: Unable to connect to the server.";
    }

    const char* request = "GET / HTTP/1.1\r\nHost: checkip.amazonaws.com\r\nConnection: close\r\n\r\n";
    send(sock, request, strlen(request), 0);
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate the response
        std::string response(buffer);
        size_t pos = response.find("\r\n\r\n");
        if (pos != std::string::npos) {
            ip = response.substr(pos + 4); // Extract the IP from the response
        }
    }

    closesocket(sock);
    WSACleanup();
    return ip;
}

std::string base64_encode(const unsigned char* bytes_to_encode, unsigned int in_len) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::string encodeIPToMnemonic(const std::string& ip) {
    // Convert IP to binary
    std::vector<uint8_t> binaryIP;
    std::istringstream iss(ip);
    std::string segment;
    while (std::getline(iss, segment, '.')) {
        binaryIP.push_back(static_cast<uint8_t>(std::stoi(segment)));
    }

    // Encode binary IP to base64
    std::string base64Encoded = base64_encode(binaryIP.data(), binaryIP.size());

    // Add salted words for mnemonic
    std::vector<std::string> saltWords = {"apple", "banana", "cherry", "date", "elderberry"};
    std::string mnemonic = base64Encoded;
    for (const auto& word : saltWords) {
        mnemonic += "-" + word; // Append salted words
    }

    return mnemonic;
}

const std::string encryptionKey = "your-encryption-key-32bytes!"; // 32 bytes for AES-256, base it on a hash of the computers UUID

void sendMessage(const std::string& message) {
    std::string encryptedMessage = encrypt(message, encryptionKey);
    send(sock, encryptedMessage.c_str(), encryptedMessage.length(), 0);
}

void displayMessage(const std::string& username, const std::string& message) {
    std::string currentText;
    int length = GetWindowTextLength(hMessageDisplay);
    currentText.resize(length);
    GetWindowText(hMessageDisplay, &currentText[0], length + 1);
    currentText += username + ": " + message + "\r\n"; // Use "\r\n" for proper line breaks in Windows
    SetWindowText(hMessageDisplay, currentText.c_str());

}

void displayMnemonicCode(const std::string& mnemonic) {
    SetWindowText(hMnemonicCodeDisplay, mnemonic.c_str());
}

void fetchAndDisplayPublicIP() {
    std::string publicIP = fetchPublicIP();
    std::cout << "Public IP fetched: " << publicIP << std::endl; // Debug statement
    std::string mnemonicCode = encodeIPToMnemonic(publicIP);
    displayMnemonicCode(mnemonicCode);
}

void receiveMessages() {
    char buffer[1024];
    while (true) {
        int valread = recv(sock, buffer, sizeof(buffer), 0);
        if (valread > 0) {
            buffer[valread] = '\0'; // Null-terminate the received string
            std::string decryptedMessage = decrypt(std::string(buffer), encryptionKey);
            std::cout << "Received message: " << decryptedMessage << std::endl;
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hMessageDisplay = CreateWindow("EDIT", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_BORDER,
                10, 10, 600, 400,
                hwnd, NULL, NULL, NULL);

            hMessageInput = CreateWindow("EDIT", NULL,
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
                10, 420, 400, 25,
                hwnd, NULL, NULL, NULL);

            CreateWindow("BUTTON", "Send",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                420, 420, 80, 25,
                hwnd, (HMENU)1, NULL, NULL);

            hMnemonicInput = CreateWindow("EDIT", NULL,
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
                10, 460, 400, 25,
                hwnd, NULL, NULL, NULL);

            CreateWindow("BUTTON", "Join Group",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                420, 460, 150, 25,
                hwnd, (HMENU)2, NULL, NULL);

            CreateWindow("BUTTON", "Create Group Code",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                10, 500, 150, 25,
                hwnd, (HMENU)3, NULL, NULL);

            hMnemonicCodeDisplay = CreateWindow("EDIT", NULL,
                WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
                10, 540, 600, 25,
                hwnd, NULL, NULL, NULL);
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) { // Send button clicked
                char message[256];
                GetWindowText(hMessageInput, message, sizeof(message));
                sendMessage(message);
                displayMessage("You", message); // Display with "You" as username
                SetWindowText(hMessageInput, ""); // Clear input
            } else if (LOWORD(wParam) == 2) { // Join Mnemonic IP Group button clicked
                char mnemonic[256];
                GetWindowText(hMnemonicInput, mnemonic, sizeof(mnemonic));
                // Logic to join the group using the mnemonic IP
                std::string joinMessage = "Joining group: " + std::string(mnemonic);
                sendMessage(joinMessage); // Send the join message to the server
                SetWindowText(hMnemonicInput, ""); // Clear input
                SetWindowText(hMessageInput, ""); // Clear message input field
            } else if (LOWORD(wParam) == 3) { // Generate Mnemonic Code button clicked
                std::thread(fetchAndDisplayPublicIP).detach(); // Run in a separate thread
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void startServer() {
    WSADATA wsaData;
    SOCKET server_sock, client_sock;
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
        int valread = recv(client_sock, buffer, 1024, 0);
        if (valread <= 0) {
            std::cerr << "Connection closed or error receiving message" << std::endl;
            break;
        }
        std::cout << "Received message: " << buffer << std::endl;
        // Echo the message back to the client
        send(client_sock, buffer, valread, 0);
    }

    // Cleanup
    closesocket(client_sock);
    closesocket(server_sock);
    WSACleanup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    const char CLASS_NAME[] = "MessengerWindowClass";

    // Input anonymous username
    std::string username;
    std::cout << "Enter a username: ";
    std::getline(std::cin, username);

    // Start the server in a separate thread
    std::thread serverThread(startServer);
    serverThread.detach(); // Detach the thread to run independently

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "./cosmic EncryPost",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 650,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nShowCmd);

    // Initialize Winsock and connect to server
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost

    connect(sock, (struct sockaddr*)&server_address, sizeof(server_address));

    // Start receiving messages in a separate thread
    std::thread(receiveMessages).detach();

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}
