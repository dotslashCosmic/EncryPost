#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <vector> // Include vector header
#include "encryption.h"

extern const std::string encryptionKey; // Declare encryptionKey from client.cpp

void sendFile(SOCKET sock, const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }

    // Get the file size
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Send the file size first
    send(sock, reinterpret_cast<const char*>(&fileSize), sizeof(fileSize), 0);

    // Send the file content
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        send(sock, buffer, file.gcount(), 0);
    }
    // Send any remaining bytes
    send(sock, buffer, file.gcount(), 0);

    file.close();
    std::cout << "File sent successfully: " << filename << std::endl;
}

void receiveFile(SOCKET& sock) {
    size_t fileSize;
    recv(sock, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    std::vector<char> buffer(fileSize);
    recv(sock, buffer.data(), fileSize, 0);

    // Decrypt the file content
    std::string decryptedContent = decrypt(std::string(buffer.data(), fileSize), encryptionKey);

    // Save the decrypted content to a file
    std::ofstream outFile("received_file", std::ios::binary);
    outFile.write(decryptedContent.c_str(), decryptedContent.size());
    outFile.close();

    std::cout << "File received and saved as 'received_file'" << std::endl;
}
