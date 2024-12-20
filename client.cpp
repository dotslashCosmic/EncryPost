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
#include <algorithm>
#include <chrono>
#include "encryption.h"
#include "file_transfer.h"

#define PORT 53100
#define KNOCK_1 53101
#define KNOCK_2 53102
#define KNOCK_3 53103

SOCKET sock;
HWND hMessageInput, hMessageDisplay, hMnemonicInput, hMnemonicCodeDisplay;

std::string sanitizeUsername(const std::string& username) {
    std::string sanitized = username;
    // Trim leading/trailing whitespace
    sanitized.erase(0, sanitized.find_first_not_of(" \t\n"));
    sanitized.erase(sanitized.find_last_not_of(" \t\n") + 1);

    // Remove unwanted characters
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](char c) { return !std::isalnum(c) && c != '_'; }), sanitized.end());
    
    // Optionally limit length
    if (sanitized.length() > 20) {
        sanitized = sanitized.substr(0, 20);
    }

    // Reject empty input
    if (sanitized.empty()) {
        throw std::invalid_argument("Username cannot be empty.");
    }

    return sanitized;
}

std::string sanitizeMessage(const std::string& message) {
    std::string sanitizedMessage = message; // Create a copy for sanitization
    if (sanitizedMessage.empty()) {
        throw std::invalid_argument("Message cannot be empty.");
    }

    sanitizedMessage.erase(0, sanitizedMessage.find_first_not_of(" \t\n")); // Leading whitespace
    sanitizedMessage.erase(sanitizedMessage.find_last_not_of(" \t\n") + 1); // Trailing whitespace

    for (char c : message) {
        // Encode special characters to prevent code execution
        switch (c) {
            case '<': sanitizedMessage += "&lt;"; break;
            case '>': sanitizedMessage += "&gt;"; break;
            case '&': sanitizedMessage += "&amp;"; break;
            case '"': sanitizedMessage += "&quot;"; break;
            case '\'': sanitizedMessage += "&#39;"; break;
            default: sanitizedMessage += c; // Keep all other characters
        }
    }
    return sanitizedMessage;
}

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

std::string encrypt(const std::string& plaintext, const std::string& key);
std::string decrypt(const std::string& ciphertext, const std::string& key);
std::string encode_ip(const std::string& ip);
std::string decode_ip(const std::string& mnemonic);

void sendMessage(const std::string& message) {
    // Sanitize message
    std::string sanitizedMessage = sanitizeMessage(message);
    if (sanitizedMessage.empty()) {
        std::cerr << "Cannot send an empty message." << std::endl;
        return; // Prevent sending empty messages
    }
    std::string encryptedMessage = encrypt(message, encryptionKey);
    send(sock, encryptedMessage.c_str(), encryptedMessage.length(), 0);
}

void displayMessage(const std::string& username, const std::string& message) {
    std::string currentText;
    int length = GetWindowTextLength(hMessageDisplay);
    currentText.resize(length);
    GetWindowText(hMessageDisplay, &currentText[0], length + 1);
    std::string formattedMessage = message;
    // Check if the message contains a file name (assuming it starts with " \" and ends with \"")
    if (formattedMessage.find("\"") != std::string::npos) {
        size_t start = formattedMessage.find("\"");
        size_t end = formattedMessage.find("\"", start + 1);
        if (end != std::string::npos) {
            std::string fileName = formattedMessage.substr(start + 1, end - start - 1);
            formattedMessage.replace(start, end - start + 1, fileName);
        }
    }
    currentText += username + ": " + formattedMessage + "\r\n"; // Use "\r\n" for proper line breaks in Windows

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

struct User {
    std::string username;
    int id; // Unique identifier for the user
};

std::vector<User> users; // Vector to store connected users
std::vector<int> knockSequence = {KNOCK_1, KNOCK_2, KNOCK_3};
std::vector<int> receivedKnocks;

std::chrono::steady_clock::time_point knockStartTime;
const int KNOCK_TIMEOUT_SECONDS = 10; // Timeout for knock sequence

void resetKnockSequence() {
    receivedKnocks.clear();
}

bool isKnockSequenceValid() {
    if (receivedKnocks.size() != knockSequence.size()) {
        return false;
    }
    return std::equal(receivedKnocks.begin(), receivedKnocks.end(), knockSequence.begin());
}

void handleClient(SOCKET client_sock) {
    char buffer[1024] = {0};
    int userId = users.size() + 1; // Generate a unique ID
    std::string username;

    // Receive username
    int usernameLength = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (usernameLength > 0) {
        buffer[usernameLength] = '\0'; // Null-terminate the received string
        username = buffer;
        users.push_back({username, userId}); // Store user info
    }

    while (true) {
        int valread = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            std::cerr << "Connection closed or error receiving message" << std::endl;
            break;
        }
        
        buffer[valread] = '\0'; // Null-terminate the received string
        std::cout << "Received message from " << username << ": " << buffer << std::endl;
        // Echo the message back to the client
        send(client_sock, buffer, valread, 0);
    }
    closesocket(client_sock);
}

void handleKnock(SOCKET knock_client_sock, struct sockaddr_in client_address, SOCKET server_sock, struct sockaddr_in server_address) {
    receivedKnocks.push_back(ntohs(client_address.sin_port)); // Store the received knock port

    // Check if the knock sequence is valid
    if (isKnockSequenceValid()) {
        std::cout << "Valid knock sequence received. Opening main port for connections.\n";
        server_address.sin_port = htons(PORT);
        listen(server_sock, 3); // Reuse the same socket for main connections
        char usernameBuffer[1024] = {0};
        int usernameLength = recv(knock_client_sock, usernameBuffer, sizeof(usernameBuffer) - 1, 0);
        if (usernameLength > 0) {
            usernameBuffer[usernameLength] = '\0'; // Null-terminate the received string
        }
        std::string username(usernameBuffer);
        std::string message = username + " wishes to join the group. Accept? (y/n)";
        send(knock_client_sock, message.c_str(), message.length(), 0);
    } else {
        // Check for timeout
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - knockStartTime).count() > KNOCK_TIMEOUT_SECONDS) {
            std::cout << "Knock sequence timed out. Resetting...\n";
            resetKnockSequence();
        }
    }
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
    std::cout << "Server listening for port knocks...\n";

    // Accept connections in a loop
    while (true) {
        SOCKET knock_client_sock = accept(server_sock, (struct sockaddr *)&client_address, &addr_len);
        if (knock_client_sock < 0) {
            perror("Accept failed");
            continue; 
        }

        knockStartTime = std::chrono::steady_clock::now(); // Start the timer for knock sequence
        handleKnock(knock_client_sock, client_address, server_sock, server_address);
        closesocket(knock_client_sock);
    }

    // Cleanup
    closesocket(server_sock);
    WSACleanup();
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

            CreateWindow("BUTTON", "Upload File",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                500, 420, 80, 25,
                hwnd, (HMENU)4, NULL, NULL); // Add button for file uploading

            hMnemonicCodeDisplay = CreateWindow("EDIT", NULL,
                WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
                10, 540, 600, 25,
                hwnd, NULL, NULL, NULL);
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 4) { // Upload File button clicked
        // Logic to open a file dialog and send the selected file
            OPENFILENAME ofn;       // common dialog box structure
            char szFile[260];       // buffer for file name

        // Initialize OPENFILENAME
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        // Display the file dialog box
            if (GetOpenFileName(&ofn)) {
                std::string fullPath = ofn.lpstrFile; // Get the full path
                std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1); // Extract the file name
                sendFile(sock, filename.c_str()); // Send the selected file
                displayMessage("You sent", std::string(" \"") + filename + "\""); // Display file sent message
            }
            break;
        }
        
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    const char CLASS_NAME[] = "MessengerWindowClass";

    // Input anonymous username
    std::string username;
    std::cout << "Enter a username: ";
    std::getline(std::cin, username);
    username = sanitizeUsername(username); // Sanitize the username
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
