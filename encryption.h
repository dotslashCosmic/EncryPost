#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

extern const std::string encryptionKey; // Declare encryptionKey as extern

std::string encrypt(const std::string& plaintext, const std::string& key);
std::string decrypt(const std::string& ciphertext, const std::string& key);
std::string encode_ip(const std::string& ip);
std::string decode_ip(const std::string& mnemonic);

#endif // ENCRYPTION_H
