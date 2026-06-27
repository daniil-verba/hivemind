// src/network/BeaconServer.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <csignal>

enum class MsgType : uint8_t {
    MESSAGE = 0,
    PING = 1,
    PONG = 2,
    REGISTER_USERNAME = 10,
    RESOLVE_USERNAME = 11,
    USERNAME_RESPONSE = 12,
    BOOTSTRAP_REQUEST = 20,
    BOOTSTRAP_RESPONSE = 21,
    BEACON_HEARTBEAT = 30,
    BEACON_STATUS = 31,
    REQUEST_USERS_PACK = 40,
    USERS_PACK_RESPONSE = 41,
    RELAY_MESSAGE = 50,
    RELAY_DELIVERY = 51,
};

struct Packet {
    MsgType type;
    std::vector<uint8_t> payload;
    
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        result.reserve(1 + payload.size());
        result.push_back(static_cast<uint8_t>(type));
        result.insert(result.end(), payload.begin(), payload.end());
        return result;
    }
    
    static bool deserialize(const uint8_t* data, size_t len, Packet& out) {
        if (len < 1) return false;
        out.type = static_cast<MsgType>(data[0]);
        out.payload.assign(data + 1, data + len);
        return true;
    }
};

struct NodeInfo {
    uint8_t nodeId[20];
    char username[32];
    char publicIp[16];
    char localIp[16];
    uint16_t port;
    uint64_t timestamp;
    uint8_t signature[64];
};

// === USERS.PACK РЕЕСТР ===
struct NetworkUser {
    std::string beaconIp;
    std::string username;
    std::string nodeId;
    std::string publicIp;
    uint16_t port;
    std::chrono::system_clock::time_point lastSeen;
};

class UsersPackRegistry {
public:
    UsersPackRegistry(const std::string& filePath) : m_filePath(filePath) {
        load();
    }
    
    bool load() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ifstream file(m_filePath);
        if (!file.is_open()) {
            std::cout << "[BeaconServer] Creating new users.pack" << std::endl;
            return true;
        }
        
        m_users.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::stringstream ss(line);
            std::string beaconIp, username, nodeId, publicIp, portStr;
            
            if (std::getline(ss, beaconIp, '|') &&
                std::getline(ss, username, '|') &&
                std::getline(ss, nodeId, '|') &&
                std::getline(ss, publicIp, '|') &&
                std::getline(ss, portStr)) {
                NetworkUser user;
                user.beaconIp = beaconIp;
                user.username = username;
                user.nodeId = nodeId;
                user.publicIp = publicIp;
                user.port = static_cast<uint16_t>(std::stoi(portStr));
                user.lastSeen = std::chrono::system_clock::now();
                m_users[nodeId] = user;
            }
        }
        
        std::cout << "[BeaconServer] Loaded " << m_users.size() << " users from " << m_filePath << std::endl;
        return true;
    }
    
    bool save() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ofstream file(m_filePath, std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "[BeaconServer] Failed to save " << m_filePath << std::endl;
            return false;
        }
        
        file << "# users.pack - Hivemind P2P Registry\\n";
        file << "# Format: BEACON_IP|USERNAME|NODE_ID|PUBLIC_IP|PORT\\n";
        
        for (const auto& pair : m_users) {
            const auto& user = pair.second;
            file << user.beaconIp << "|" << user.username << "|" << user.nodeId << "|"
                 << user.publicIp << "|" << user.port << "\\n";
        }
        
        std::cout << "[BeaconServer] Saved " << m_users.size() << " users to " << m_filePath << std::endl;
        return true;
    }
    
    bool addOrUpdateUser(const std::string& beaconIp, const std::string& username,
                          const std::string& nodeId, const std::string& publicIp, uint16_t port) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_users.find(nodeId);
        if (it != m_users.end()) {
            // Обновляем существующего
            it->second.beaconIp = beaconIp;
            it->second.username = username;
            it->second.publicIp = publicIp;
            it->second.port = port;
            it->second.lastSeen = std::chrono::system_clock::now();
            std::cout << "[BeaconServer] Updated user: " << username << " (" << nodeId << ")" << std::endl;
        } else {
            // Новый пользователь
            NetworkUser user;
            user.beaconIp = beaconIp;
            user.username = username;
            user.nodeId = nodeId;
            user.publicIp = publicIp;
            user.port = port;
            user.lastSeen = std::chrono::system_clock::now();
            m_users[nodeId] = user;
            std::cout << "[BeaconServer] New user registered: " << username << " (" << nodeId << ")" << std::endl;
        }
        
        return save();
    }
    
    bool findByUsername(const std::string& username, NetworkUser& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& pair : m_users) {
            if (pair.second.username == username) {
                out = pair.second;
                return true;
            }
        }
        return false;
    }
    
    bool findByNodeId(const std::string& nodeId, NetworkUser& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_users.find(nodeId);
        if (it != m_users.end()) {
            out = it->second;
            return true;
        }
        return false;
    }
    
    std::vector<std::string> getAllLines() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> lines;
        for (const auto& pair : m_users) {
            const auto& user = pair.second;
            std::stringstream ss;
            ss << user.beaconIp << "|" << user.username << "|" << user.nodeId << "|"
               << user.publicIp << "|" << user.port;
            lines.push_back(ss.str());
        }
        return lines;
    }
    
    std::vector<NetworkUser> getAllUsers() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<NetworkUser> result;
        for (const auto& pair : m_users) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    void printUsers() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[BeaconServer] Registered users (" << m_users.size() << "):" << std::endl;
        for (const auto& pair : m_users) {
            const auto& user = pair.second;
            std::cout << "  [" << user.beaconIp << "] " << user.username 
                      << " (" << user.nodeId << ") -> " << user.publicIp << ":" << user.port << std::endl;
        }
    }
    
private:
    std::string m_filePath;
    std::map<std::string, NetworkUser> m_users;
    mutable std::mutex m_mutex;
};

// === UDP СЕРВЕР ===
class BeaconServer {
public:
    BeaconServer(uint16_t port, const std::string& usersPackPath) 
        : m_port(port), m_registry(usersPackPath), m_running(false) {
        // Получаем свой IP
        m_myIp = getMyIp();
        std::cout << "[BeaconServer] My IP: " << m_myIp << std::endl;
    }
    
    bool start() {
        m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_sockfd < 0) {
            std::cerr << "[BeaconServer] Failed to create socket" << std::endl;
            return false;
        }
        
        int opt = 1;
        setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(m_port);
        
        if (bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[BeaconServer] Failed to bind to port " << m_port << std::endl;
            close(m_sockfd);
            return false;
        }
        
        m_running = true;
        std::cout << "[BeaconServer] Started on port " << m_port << std::endl;
        std::cout << "[BeaconServer] Users pack: users.pack" << std::endl;
        
        m_registry.printUsers();
        
        receiveLoop();
        return true;
    }
    
    void stop() {
        m_running = false;
        if (m_sockfd >= 0) {
            close(m_sockfd);
            m_sockfd = -1;
        }
    }
    
private:
    std::string getMyIp() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return "0.0.0.0";
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(80);
        inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return "0.0.0.0";
        }
        
        struct sockaddr_in localAddr;
        socklen_t addrLen = sizeof(localAddr);
        if (getsockname(sock, (struct sockaddr*)&localAddr, &addrLen) < 0) {
            close(sock);
            return "0.0.0.0";
        }
        
        close(sock);
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddr.sin_addr, ip, INET_ADDRSTRLEN);
        return std::string(ip);
    }
    
    void receiveLoop() {
        char buffer[65536];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        std::cout << "[BeaconServer] Receive loop started, waiting for packets..." << std::endl;
        
        while (m_running) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(m_sockfd, &fds);
            
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            int ret = select(m_sockfd + 1, &fds, nullptr, nullptr, &tv);
            if (ret <= 0) continue;
            
            int received = recvfrom(m_sockfd, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
            if (received <= 0) continue;
            
            buffer[received] = '\0';
            
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
            uint16_t port = ntohs(from_addr.sin_port);
            
            // ЛОГИРУЕМ ВСЕ ВХОДЯЩИЕ ПАКЕТЫ
            std::cout << "[BeaconServer] >>> RECEIVED " << received << " bytes from " 
                      << ip_str << ":" << port << std::endl;
            
            Packet pkt;
            if (!Packet::deserialize(reinterpret_cast<uint8_t*>(buffer), received, pkt)) {
                std::cout << "[BeaconServer]   Failed to deserialize packet" << std::endl;
                continue;
            }
            
            std::cout << "[BeaconServer]   Packet type: " << static_cast<int>(pkt.type) 
                      << " (size: " << pkt.payload.size() << ")" << std::endl;
            
            processPacket(pkt, ip_str, port, from_addr);
        }
    }
    
    void processPacket(const Packet& pkt, const std::string& fromIp, uint16_t fromPort,
                       const struct sockaddr_in& fromAddr) {
        switch (pkt.type) {
            /// === РЕГИСТРАЦИЯ ПОЛЬЗОВАТЕЛЯ ===
            case MsgType::REGISTER_USERNAME: {
                std::cout << "[BeaconServer] >>> GOT REGISTER_USERNAME from " << fromIp << ":" << fromPort << std::endl;
                std::cout << "[BeaconServer]   Payload size: " << pkt.payload.size() << " bytes" << std::endl;
                
                if (pkt.payload.size() < 32 + 158) {
                    std::cerr << "[BeaconServer] Invalid REGISTER_USERNAME packet (too small: " << pkt.payload.size() << ")" << std::endl;
                    return;
                }
                
                std::string username(pkt.payload.begin(), pkt.payload.begin() + 32);
                // Убираем null-терминаторы
                size_t nullPos = username.find('\0');
                if (nullPos != std::string::npos) username = username.substr(0, nullPos);
                
                // Парсим NodeInfo
                const uint8_t* nodeData = pkt.payload.data() + 32;
                NodeInfo info{};
                memcpy(info.nodeId, nodeData, 20);
                memcpy(info.username, nodeData + 20, 32);
                info.username[31] = '\0';
                memcpy(info.publicIp, nodeData + 52, 16);
                info.publicIp[15] = '\0';
                memcpy(info.localIp, nodeData + 68, 16);
                info.localIp[15] = '\0';
                info.port = ntohs((nodeData[84] << 8) | nodeData[85]);
                
                // Конвертируем nodeId в hex
                std::stringstream nodeIdSs;
                for (int i = 0; i < 20; ++i) {
                    nodeIdSs << std::hex << std::setw(2) << std::setfill('0') << (int)info.nodeId[i];
                }
                std::string nodeId = nodeIdSs.str();
                
                std::string publicIp(info.publicIp);
                // Убираем null-терминаторы
                nullPos = publicIp.find('\0');
                if (nullPos != std::string::npos) publicIp = publicIp.substr(0, nullPos);
                
                // Добавляем/обновляем пользователя
                bool success = m_registry.addOrUpdateUser(m_myIp, username, nodeId, publicIp, info.port);
                
                // Отправляем ответ
                Packet response;
                response.type = MsgType::USERNAME_RESPONSE;
                response.payload.push_back(success ? 1 : 0);
                if (success) {
                    auto nodeData = serializeNodeInfo(info);
                    response.payload.insert(response.payload.end(), nodeData.begin(), nodeData.end());
                }
                auto data = response.serialize();
                sendto(m_sockfd, data.data(), data.size(), 0,
                       (struct sockaddr*)&fromAddr, sizeof(fromAddr));
                
                std::cout << "[BeaconServer] Registered: " << username << " -> " << publicIp << ":" << info.port << std::endl;
                break;
            }
            
            // === ЗАПРОС USERS.PACK ===
            case MsgType::REQUEST_USERS_PACK: {
                std::string requesterNodeId(pkt.payload.begin(), pkt.payload.end());
                std::cout << "[BeaconServer] Users pack requested by " << requesterNodeId.substr(0, 8) << "..." << std::endl;
                
                auto lines = m_registry.getAllLines();
                
                Packet response;
                response.type = MsgType::USERS_PACK_RESPONSE;
                response.payload.push_back(static_cast<uint8_t>(lines.size()));
                for (const auto& line : lines) {
                    uint16_t len = static_cast<uint16_t>(line.length());
                    response.payload.push_back((len >> 8) & 0xFF);
                    response.payload.push_back(len & 0xFF);
                    response.payload.insert(response.payload.end(), line.begin(), line.end());
                }
                
                auto data = response.serialize();
                sendto(m_sockfd, data.data(), data.size(), 0,
                       (struct sockaddr*)&fromAddr, sizeof(fromAddr));
                
                std::cout << "[BeaconServer] Sent users.pack (" << lines.size() << " users)" << std::endl;
                break;
            }
            
            // === РЕТРАНСЛЯЦИЯ СООБЩЕНИЯ ===
            case MsgType::RELAY_MESSAGE: {
                if (pkt.payload.size() < 82) {
                    std::cerr << "[BeaconServer] Invalid RELAY_MESSAGE packet" << std::endl;
                    return;
                }
                
                std::string fromNodeId(pkt.payload.begin(), pkt.payload.begin() + 40);
                std::string toNodeId(pkt.payload.begin() + 40, pkt.payload.begin() + 80);
                uint16_t msgLen = (pkt.payload[80] << 8) | pkt.payload[81];
                std::string message(pkt.payload.begin() + 82, pkt.payload.begin() + 82 + msgLen);
                
                std::cout << "[BeaconServer] Relay message from " << fromNodeId.substr(0, 8) 
                          << "... to " << toNodeId.substr(0, 8) << "..." << std::endl;
                
                // Находим получателя
                NetworkUser target;
                if (!m_registry.findByNodeId(toNodeId, target)) {
                    std::cerr << "[BeaconServer] Target not found: " << toNodeId << std::endl;
                    return;
                }
                
                // Пересылаем сообщение получателю
                struct sockaddr_in targetAddr;
                memset(&targetAddr, 0, sizeof(targetAddr));
                targetAddr.sin_family = AF_INET;
                targetAddr.sin_port = htons(target.port);
                inet_pton(AF_INET, target.publicIp.c_str(), &targetAddr.sin_addr);
                
                auto data = pkt.serialize();
                int sent = sendto(m_sockfd, data.data(), data.size(), 0,
                                  (struct sockaddr*)&targetAddr, sizeof(targetAddr));
                
                if (sent > 0) {
                    std::cout << "[BeaconServer] Relayed to " << target.publicIp << ":" << target.port << std::endl;
                    
                    // Отправляем подтверждение отправителю
                    Packet ack;
                    ack.type = MsgType::RELAY_DELIVERY;
                    ack.payload.push_back(1); // success
                    ack.payload.insert(ack.payload.end(), toNodeId.begin(), toNodeId.end());
                    auto ackData = ack.serialize();
                    sendto(m_sockfd, ackData.data(), ackData.size(), 0,
                           (struct sockaddr*)&fromAddr, sizeof(fromAddr));
                } else {
                    std::cerr << "[BeaconServer] Failed to relay message" << std::endl;
                }
                break;
            }
            
            // === РАЗРЕШЕНИЕ ИМЕНИ ===
            case MsgType::RESOLVE_USERNAME: {
                std::string username(pkt.payload.begin(), pkt.payload.begin() + 32);
                size_t nullPos = username.find('\0');
                if (nullPos != std::string::npos) username = username.substr(0, nullPos);
                
                NetworkUser user;
                bool found = m_registry.findByUsername(username, user);
                
                Packet response;
                response.type = MsgType::USERNAME_RESPONSE;
                response.payload.push_back(found ? 1 : 0);
                
                if (found) {
                    NodeInfo info{};
                    // Конвертируем hex nodeId обратно в bytes
                    for (int i = 0; i < 20; ++i) {
                        std::string byteStr = user.nodeId.substr(i * 2, 2);
                        info.nodeId[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
                    }
                    strncpy(info.username, user.username.c_str(), 31);
                    info.username[31] = '\0';
                    strncpy(info.publicIp, user.publicIp.c_str(), 15);
                    info.publicIp[15] = '\0';
                    strncpy(info.localIp, user.publicIp.c_str(), 15); // используем public как local
                    info.localIp[15] = '\0';
                    info.port = user.port;
                    info.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                    
                    auto nodeData = serializeNodeInfo(info);
                    response.payload.insert(response.payload.end(), nodeData.begin(), nodeData.end());
                }
                
                auto data = response.serialize();
                sendto(m_sockfd, data.data(), data.size(), 0,
                       (struct sockaddr*)&fromAddr, sizeof(fromAddr));
                break;
            }
            
            // === BOOTSTRAP ===
            case MsgType::BOOTSTRAP_REQUEST: {
                auto users = m_registry.getAllUsers();
                
                Packet response;
                response.type = MsgType::BOOTSTRAP_RESPONSE;
                response.payload.push_back(static_cast<uint8_t>(std::min(users.size(), size_t(255))));
                
                int count = 0;
                for (const auto& user : users) {
                    if (count >= 255) break;
                    
                    NodeInfo info{};
                    for (int i = 0; i < 20; ++i) {
                        std::string byteStr = user.nodeId.substr(i * 2, 2);
                        info.nodeId[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
                    }
                    strncpy(info.username, user.username.c_str(), 31);
                    info.username[31] = '\0';
                    strncpy(info.publicIp, user.publicIp.c_str(), 15);
                    info.publicIp[15] = '\0';
                    strncpy(info.localIp, user.publicIp.c_str(), 15);
                    info.localIp[15] = '\0';
                    info.port = user.port;
                    info.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                    
                    auto nodeData = serializeNodeInfo(info);
                    response.payload.insert(response.payload.end(), nodeData.begin(), nodeData.end());
                    count++;
                }
                
                auto data = response.serialize();
                sendto(m_sockfd, data.data(), data.size(), 0,
                       (struct sockaddr*)&fromAddr, sizeof(fromAddr));
                break;
            }
            
            case MsgType::BEACON_HEARTBEAT: {
                Packet response;
                response.type = MsgType::BEACON_STATUS;
                auto data = response.serialize();
                sendto(m_sockfd, data.data(), data.size(), 0,
                       (struct sockaddr*)&fromAddr, sizeof(fromAddr));
                break;
            }
            
            default:
                break;
        }
    }
    
    std::vector<uint8_t> serializeNodeInfo(const NodeInfo& info) {
        std::vector<uint8_t> result;
        result.reserve(158);
        result.insert(result.end(), info.nodeId, info.nodeId + 20);
        result.insert(result.end(), reinterpret_cast<const uint8_t*>(info.username),
                      reinterpret_cast<const uint8_t*>(info.username + 32));
        result.insert(result.end(), reinterpret_cast<const uint8_t*>(info.publicIp),
                      reinterpret_cast<const uint8_t*>(info.publicIp + 16));
        result.insert(result.end(), reinterpret_cast<const uint8_t*>(info.localIp),
                      reinterpret_cast<const uint8_t*>(info.localIp + 16));
        uint16_t port_net = htons(info.port);
        result.push_back((port_net >> 8) & 0xFF);
        result.push_back(port_net & 0xFF);
        uint64_t ts_net = htobe64(info.timestamp);
        for (int i = 7; i >= 0; --i) {
            result.push_back((ts_net >> (i * 8)) & 0xFF);
        }
        result.insert(result.end(), info.signature, info.signature + 64);
        return result;
    }
    
    uint16_t m_port;
    int m_sockfd;
    std::string m_myIp;
    UsersPackRegistry m_registry;
    bool m_running;
};

// === MAIN ===
int main(int argc, char* argv[]) {
    uint16_t port = 7777;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }
    
    std::cout << "═══════════════════════════════════════════════════" << std::endl;
    std::cout << "  HIVEMIND BEACON SERVER v0.1" << std::endl;
    std::cout << "  Port: " << port << std::endl;
    std::cout << "  Registry: users.pack" << std::endl;
    std::cout << "═══════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    
    BeaconServer server(port, "users.pack");
    
    // Обработка сигналов
    signal(SIGINT, [](int) {
        std::cout << "\\n[BeaconServer] Shutting down..." << std::endl;
        exit(0);
    });
    
    server.start();
    
    return 0;
}