#include <string>
#include <sstream>
#include <iomanip>

// Function to encode an IP address into a mnemonic format
std::string encode_ip(const std::string& ip) {
    std::stringstream ss;
    for (const char& c : ip) {
        ss << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return ss.str();
}

// Function to decode a mnemonic format back into an IP address
std::string decode_ip(const std::string& mnemonic) {
    std::string ip;
    for (size_t i = 0; i < mnemonic.length(); i += 2) {
        std::string byte = mnemonic.substr(i, 2);
        char chr = static_cast<char>(std::stoi(byte, nullptr, 16));
        ip += chr;
    }
    return ip;
}
