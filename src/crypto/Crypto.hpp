#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Crypto {
    void init();
    void generateKeyPair(std::vector<uint8_t>& pubKey, std::vector<uint8_t>& privKey);
    std::vector<uint8_t> deriveSharedSecret(const std::vector<uint8_t>& myPrivKey, 
                                             const std::vector<uint8_t>& theirPubKey);
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& key, const std::string& plaintext);
    std::string decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& ciphertext);
}