#ifndef MNEMONIC_UTILS_H
#define MNEMONIC_UTILS_H

#include <string>

std::string encode_ip(const std::string& ip);
std::string encodeIPToMnemonic(const std::string& ip);
std::string decode_ip(const std::string& mnemonic);
std::string decodeBase64(const std::string& encoded_string);
std::string binaryToIP(const std::vector<uint8_t>& binaryIP);
std::string base64_encode(const unsigned char* bytes_to_encode, unsigned int in_len);

#endif // MNEMONIC_UTILS_H
