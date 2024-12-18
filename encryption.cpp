#include "encryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

std::string encrypt(const std::string& plaintext, const std::string& key) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::string ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int len;

    // Initialize encryption
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.c_str(), NULL);
    EVP_EncryptUpdate(ctx, (unsigned char*)&ciphertext[0], &len, (unsigned char*)plaintext.c_str(), plaintext.size());
    int ciphertext_len = len;

    // Finalize encryption
    EVP_EncryptFinal_ex(ctx, (unsigned char*)&ciphertext[0] + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext.substr(0, ciphertext_len);
}

std::string decrypt(const std::string& ciphertext, const std::string& key) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::string plaintext(ciphertext.size(), '\0');
    int len;

    // Initialize decryption
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.c_str(), NULL);
    EVP_DecryptUpdate(ctx, (unsigned char*)&plaintext[0], &len, (unsigned char*)ciphertext.c_str(), ciphertext.size());
    int plaintext_len = len;

    // Finalize decryption
    EVP_DecryptFinal_ex(ctx, (unsigned char*)&plaintext[0] + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext.substr(0, plaintext_len);
}
