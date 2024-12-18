#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

std::string encrypt(const std::string& plaintext, const std::string& key);
std::string decrypt(const std::string& ciphertext, const std::string& key);
std::string encode_ip(const std::string& ip);
std::string decode_ip(const std::string& mnemonic);

#endif // ENCRYPTION_H
