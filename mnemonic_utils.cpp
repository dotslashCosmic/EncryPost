#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <unordered_map>
#include <string>

std::string base64_encode(const unsigned char* bytes_to_encode, unsigned int in_len);
std::string decodeBase64(const std::string& encoded_string);
std::string binaryToIP(const std::vector<uint8_t>& binaryIP);

std::string decodeMnemonicToIP(const std::string& mnemonic) {
    // Split the mnemonic to separate the base64 part and the salt words
    size_t pos = mnemonic.find('-');
    std::string base64Part = (pos != std::string::npos) ? mnemonic.substr(0, pos) : mnemonic;

    // Decode base64 to binary
    std::string binaryData = decodeBase64(base64Part);
    std::vector<uint8_t> binaryIP(binaryData.begin(), binaryData.end());

    // Convert binary to IP
    return binaryToIP(binaryIP);
}
std::string encodeIPToMnemonic(const std::string& ip) {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(0)));

    // List of potential salt words
    std::vector<std::string> saltWords = {"apple", "banana", "cherry", "date", "elderberry", "fig", "grape", "honeydew"};
    std::string randomSaltWord = saltWords[std::rand() % saltWords.size()]; // Select a random salt word

    // Convert IP to binary
    std::vector<uint8_t> binaryIP;
    std::istringstream iss(ip);
    std::string segment;
    while (std::getline(iss, segment, '.')) {
        binaryIP.push_back(static_cast<uint8_t>(std::stoi(segment)));
    }

    // Encode binary IP to base64
    std::string base64Encoded = base64_encode(binaryIP.data(), binaryIP.size());

    // Create a mapping from characters to mnemonic words
    std::unordered_map<char, std::string> charToMnemonic = {
        {'a', "apple"}, {'b', "banana"}, {'c', "cherry"}, {'d', "date"},
        {'e', "endive"}, {'f', "fig"}, {'g', "grape"}, {'h', "hummus"},
        {'i', "ice"}, {'j', "jam"}, {'k', "kiwi"}, {'l', "lemon"},
        {'m', "mango"}, {'n', "nutmeg"}, {'o', "orange"}, {'p', "papaya"},
        {'q', "quinoa"}, {'r', "raisin"}, {'s', "sorbet"}, {'t', "tomato"},
        {'u', "ugli"}, {'v', "vanilla"}, {'w', "walnut"}, {'x', "xigua"},
        {'y', "yogurt"}, {'z', "zucchini"},
        {'A', "avocado"}, {'B', "beet"}, {'C', "carrot"}, {'D', "donut"},
        {'E', "egg"}, {'F', "fennel"}, {'G', "guava"}, {'H', "sugar"},
        {'I', "ice"}, {'J', "jalapeno"}, {'K', "kale"}, {'L', "lime"},
        {'M', "melon"}, {'N', "nachos"}, {'O', "olive"}, {'P', "plum"},
        {'Q', "quiche"}, {'R', "radish"}, {'S', "spinach"}, {'T', "tofu"},
        {'U', "udon"}, {'V', "veal"}, {'W', "wasabi"}, {'X', "xigua"},
        {'Y', "yuzu"}, {'Z', "ziti"}, {'0', "olive"}, {'1', "plum"},
        {'2', "mulberry"}, {'3', "peach"}, {'4', "thyme"}, {'5', "goose"},
        {'6', "pear"}, {'7', "honey"}, {'8', "basil"}, {'9', "corn"},
        {'=', "salt"}, {'+', "radish"}, {'/', "pumpkin"}
    };

    // Add uppercase mappings
    for (char c = 'A'; c <= 'Z'; ++c) {
        charToMnemonic[c] = charToMnemonic[std::tolower(c)]; // Use the same fruit for uppercase
    }

    // Create a list of mnemonic words from the base64 encoded string using the mapping
    std::vector<std::string> mnemonicWords;
    for (char c : base64Encoded) {
        if (charToMnemonic.find(c) != charToMnemonic.end()) {
            mnemonicWords.push_back(charToMnemonic[c]); // Use the mapping
        }
    }

    // Randomly insert the salt word into the mnemonic words
    int insertPosition = std::rand() % (mnemonicWords.size() + 1); // Random position
    mnemonicWords.insert(mnemonicWords.begin() + insertPosition, randomSaltWord); // Insert the salt word

    // Join the mnemonic words into a single string with dashes
    std::string mnemonic = "";
    for (size_t i = 0; i < mnemonicWords.size(); ++i) {
        mnemonic += mnemonicWords[i]; // Append each word
        if (i < mnemonicWords.size() - 1) {
            mnemonic += "-"; // Add dash between words
        }
    }
    return mnemonic;
}

std::string encode_ip(const std::string& ip) {
    return encodeIPToMnemonic(ip); // Call the function to get mnemonic words
}

std::string decode_ip(const std::string& mnemonic) {
    std::string ip;
    for (size_t i = 0; i < mnemonic.length(); i += 2) {
        std::string byte = mnemonic.substr(i, 2);
        char chr = static_cast<char>(std::stoi(byte, nullptr, 16));
        ip += chr;
    }
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

std::string decodeBase64(const std::string& encoded_string) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : encoded_string) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            ret.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return ret;
}

std::string binaryToIP(const std::vector<uint8_t>& binaryIP) {
    std::string ip;
    for (size_t i = 0; i < binaryIP.size(); ++i) {
        ip += std::to_string(binaryIP[i]);
        if (i < binaryIP.size() - 1) {
            ip += ".";
        }
    }
    return ip;
}
