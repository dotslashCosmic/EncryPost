#ifndef MNEMONIC_UTILS_H
#define MNEMONIC_UTILS_H

#include <string>

// Function to encode an IP address into a mnemonic format
std::string encode_ip(const std::string& ip);

// Function to decode a mnemonic format back into an IP address
std::string decode_ip(const std::string& mnemonic);

#endif // MNEMONIC_UTILS_H
