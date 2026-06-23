// src/network/Stun.cpp
#include "Stun.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <random>
#include <iostream>
#include <chrono>

// ============================================================
// Вспомогательные функции
// ============================================================

bool StunClient::resolveHost(const std::string& host, std::string& ip) {
    struct hostent* he = gethostbyname(host.c_str());
    if (!he) return false;
    
    struct in_addr** addrList = (struct in_addr**)he->h_addr_list;
    if (!addrList || !addrList[0]) return false;
    
    ip = inet_ntoa(*addrList[0]);
    return true;
}

// ============================================================
// STUN-запрос
// ============================================================

std::string StunClient::getPublicIPViaStun(const std::string& server, uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[STUN] Failed to create socket" << std::endl;
        return "";
    }
    
    // Разрешаем домен
    std::string serverIP;
    if (!resolveHost(server, serverIP)) {
        std::cerr << "[STUN] Failed to resolve: " << server << std::endl;
        close(sock);
        return "";
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP.c_str(), &addr.sin_addr);
    
    // ===== STUN Binding Request =====
    #pragma pack(push, 1)
    struct StunHeader {
        uint16_t type;
        uint16_t length;
        uint32_t magic;
        uint8_t transaction_id[12];
    };
    
    struct StunAttr {
        uint16_t type;
        uint16_t length;
        uint8_t value[0];
    };
    #pragma pack(pop)
    
    StunHeader req;
    req.type = htons(0x0001);  // Binding Request
    req.length = 0;
    req.magic = htonl(0x2112A442);
    
    std::random_device rd;
    for (int i = 0; i < 12; ++i) {
        req.transaction_id[i] = rd() & 0xFF;
    }
    
    // Отправляем запрос
    if (sendto(sock, &req, sizeof(req), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[STUN] Failed to send to " << server << std::endl;
        close(sock);
        return "";
    }
    
    // Устанавливаем таймаут (3 секунды)
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Ждём ответ
    char buffer[1024];
    struct sockaddr_in from;
    socklen_t fromLen = sizeof(from);
    
    int received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr*)&from, &fromLen);
    close(sock);
    
    if (received < (int)sizeof(StunHeader)) {
        std::cerr << "[STUN] No response from " << server << std::endl;
        return "";
    }
    
    // Парсим ответ
    StunHeader* resp = (StunHeader*)buffer;
    if (ntohs(resp->type) != 0x0101) {  // Binding Response
        std::cerr << "[STUN] Invalid response type from " << server << std::endl;
        return "";
    }
    
    // Ищем атрибут XOR-MAPPED-ADDRESS (тип 0x0020)
    uint8_t* ptr = (uint8_t*)(resp + 1);
    int len = ntohs(resp->length);
    
    while (len > 0) {
        StunAttr* attr = (StunAttr*)ptr;
        uint16_t attr_len = ntohs(attr->length);
        uint16_t attr_type = ntohs(attr->type);
        
        if (attr_type == 0x0020) {
            // XOR-MAPPED-ADDRESS
            uint8_t family = attr->value[1];
            if (family == 0x01) {  // IPv4
                uint32_t ip;
                memcpy(&ip, attr->value + 4, 4);
                ip ^= htonl(0x2112A442);  // XOR с магическим cookie
                
                struct in_addr addr_in;
                addr_in.s_addr = ip;
                std::string result = std::string(inet_ntoa(addr_in));
                std::cout << "[STUN] Got IP from " << server << ": " << result << std::endl;
                return result;
            }
        }
        
        ptr += 4 + attr_len;
        len -= 4 + attr_len;
    }
    
    std::cerr << "[STUN] No XOR-MAPPED-ADDRESS in response from " << server << std::endl;
    return "";
}

// ============================================================
// Получение локального IP
// ============================================================

std::string StunClient::getLocalIP() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return "";
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return "";
    }
    
    struct sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    if (getsockname(sock, (struct sockaddr*)&localAddr, &addrLen) < 0) {
        close(sock);
        return "";
    }
    
    close(sock);
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &localAddr.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
}

// ============================================================
// ОСНОВНОЙ МЕТОД: Получение публичного IP
// Приоритет: Российские STUN → Google STUN → Локальный IP
// ============================================================

std::string StunClient::getPublicIP() {
    std::cout << "[STUN] Getting public IP..." << std::endl;
    
    // ============================================================
    // 1️⃣ РОССИЙСКИЕ STUN-СЕРВЕРА (приоритет)
    // ============================================================
    std::vector<std::pair<std::string, uint16_t>> russianServers = {
        {"stun.sipnet.ru", 3478},
        {"stun.ipshka.com", 3478},
        {"stun.magenta.ru", 3478},
        {"stun.niisi.ru", 3478},
        {"stun.iptel.ru", 3478}
    };
    
    std::cout << "[STUN] Trying Russian STUN servers..." << std::endl;
    for (const auto& server : russianServers) {
        std::string ip = getPublicIPViaStun(server.first, server.second);
        if (!ip.empty()) {
            std::cout << "[STUN] ✓ Got IP from Russian STUN: " << ip << std::endl;
            return ip;
        }
    }
    
    // ============================================================
    // 2️⃣ ЗАРУБЕЖНЫЕ STUN-СЕРВЕРА (Google и другие)
    // ============================================================
    std::vector<std::pair<std::string, uint16_t>> foreignServers = {
        {"stun.l.google.com", 19302},
        {"stun1.l.google.com", 19302},
        {"stun2.l.google.com", 19302},
        {"stun3.l.google.com", 19302},
        {"stun4.l.google.com", 19302},
        {"stun.stunprotocol.org", 3478},
        {"stun.ekiga.net", 3478},
        {"stun.voipstunt.com", 3478}
    };
    
    std::cout << "[STUN] Trying foreign STUN servers..." << std::endl;
    for (const auto& server : foreignServers) {
        std::string ip = getPublicIPViaStun(server.first, server.second);
        if (!ip.empty()) {
            std::cout << "[STUN] ✓ Got IP from foreign STUN: " << ip << std::endl;
            return ip;
        }
    }
    
    // ============================================================
    // 3️⃣ ЛОКАЛЬНЫЙ IP (резерв для локальной сети)
    // ============================================================
    std::cout << "[STUN] All STUN servers failed, using local IP..." << std::endl;
    std::string localIP = getLocalIP();
    if (!localIP.empty()) {
        std::cout << "[STUN] ✓ Local IP: " << localIP << std::endl;
        return localIP;
    }
    
    std::cerr << "[STUN] ❌ Failed to get any IP" << std::endl;
    return "";
}