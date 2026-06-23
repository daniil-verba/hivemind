#include "Crypto.hpp"
#include <sodium.h>
#include <stdexcept>
#include <cstring>

namespace Crypto {

void init() {
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium_init failed");
    }
}

void generateKeyPair(std::vector<uint8_t>& pubKey, std::vector<uint8_t>& privKey) {
    pubKey.resize(crypto_box_PUBLICKEYBYTES);
    privKey.resize(crypto_box_SECRETKEYBYTES);
    crypto_box_keypair(pubKey.data(), privKey.data());
}

std::vector<uint8_t> deriveSharedSecret(const std::vector<uint8_t>& myPrivKey,
                                         const std::vector<uint8_t>& theirPubKey) {
    std::vector<uint8_t> sharedSecret(crypto_box_BEFORENMBYTES);
    if (crypto_box_beforenm(sharedSecret.data(), theirPubKey.data(), myPrivKey.data()) != 0) {
        throw std::runtime_error("Key exchange failed");
    }
    return sharedSecret;
}

std::vector<uint8_t> encrypt(const std::vector<uint8_t>& key, const std::string& plaintext) {
    std::vector<uint8_t> nonce(crypto_box_NONCEBYTES);
    randombytes_buf(nonce.data(), nonce.size());
    
    std::vector<uint8_t> ciphertext(plaintext.size() + crypto_box_MACBYTES);
    crypto_box_easy_afternm(ciphertext.data(),
                             (const unsigned char*)plaintext.data(), plaintext.size(),
                             nonce.data(),
                             key.data());
    
    std::vector<uint8_t> result;
    result.reserve(nonce.size() + ciphertext.size());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    return result;
}

std::string decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& ciphertext) {
    if (ciphertext.size() < crypto_box_NONCEBYTES + crypto_box_MACBYTES) {
        return "";
    }
    
    std::vector<uint8_t> nonce(ciphertext.begin(),
                               ciphertext.begin() + crypto_box_NONCEBYTES);
    std::vector<uint8_t> encrypted(ciphertext.begin() + crypto_box_NONCEBYTES,
                                   ciphertext.end());
    
    std::vector<uint8_t> decrypted(encrypted.size() - crypto_box_MACBYTES);
    if (crypto_box_open_easy_afternm(decrypted.data(),
                                      encrypted.data(), encrypted.size(),
                                      nonce.data(),
                                      key.data()) != 0) {
        return "";
    }
    return std::string(reinterpret_cast<char*>(decrypted.data()), decrypted.size());
}

} // namespace Crypto