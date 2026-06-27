// src/core/Hivemind.cpp
#include "Hivemind.hpp"
#include "network/Transport.hpp"
#include "network/BeaconClient.hpp"
#include "network/Stun.hpp"
#include "network/PacketManager.hpp"
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include <sstream>
#include <ifaddrs.h>
#include <arpa/inet.h>

Hivemind::Hivemind()
    : transport(std::make_unique<Transport>())
    , registry(std::make_unique<SimpleRegistry>("users.txt"))
    , beaconClient(std::make_unique<hivemind::BeaconClient>(transport.get()))
    , m_syncRunning(false)
    , m_recvRunning(false)
    , m_syncInterval(600)
    , m_holePunchingEnabled(true)
    , m_usersPackReceived(false)
{
    registry->load();
    generateNodeId();
    transport->setHolePunchCallback([this](const std::string& ip, uint16_t port) {
        onHolePunchDetected(ip, port);
    });
}

Hivemind::~Hivemind() {
    stop();
}

void Hivemind::generateNodeId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < 20; ++i) {
        m_myNodeId[i] = static_cast<uint8_t>(dis(gen));
    }
    // Конвертируем в hex-строку
    std::stringstream ss;
    for (int i = 0; i < 20; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)m_myNodeId[i];
    }
    m_myNodeIdStr = ss.str();
}

std::string Hivemind::getMyNodeId() const {
    return m_myNodeIdStr;
}

std::string Hivemind::getMyLocalIP() const {
    struct ifaddrs* ifaddr;
    struct ifaddrs* ifa;
    std::string localIP;
    if (getifaddrs(&ifaddr) == -1) return "";
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sin->sin_addr, ip, INET_ADDRSTRLEN);
            std::string ipStr(ip);
            if (ipStr != "127.0.0.1" && !ipStr.empty() && ipStr.find("172.") != 0) {
                localIP = ipStr;
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return localIP;
}

std::string Hivemind::getMyPublicIP() const {
    return m_myPublicIP;
}

bool Hivemind::start(uint16_t port) {
    m_myPort = port;
    m_myLocalIP = getMyLocalIP();
    
    if (!transport->start(port)) {
        return false;
    }
    
    m_myPublicIP = StunClient::getPublicIP();
    
    if (m_myPublicIP.empty()) {
        std::cout << "[Hivemind] Warning: Could not determine public IP via STUN" << std::endl;
        std::cout << "[Hivemind] Will use local IP only: " << m_myLocalIP << std::endl;
        m_myPublicIP = m_myLocalIP;
    } else {
        std::cout << "[Hivemind] Public IP: " << m_myPublicIP << ":" << port << std::endl;
    }
    
    if (!m_myLocalIP.empty()) {
        std::cout << "[Hivemind] Local IP: " << m_myLocalIP << ":" << port << std::endl;
    }
    
    std::cout << "[Hivemind] Node ID: " << m_myNodeIdStr << std::endl;
    
    // Запускаем receive loop
    m_recvRunning = true;
    m_recvThread = std::thread(&Hivemind::recvLoop, this);
    
    return true;
}

void Hivemind::stop() {
    m_syncRunning = false;
    m_recvRunning = false;
    
    if (m_syncThread.joinable()) {
        m_syncThread.join();
    }
    if (m_recvThread.joinable()) {
        m_recvThread.join();
    }
    
    transport->stop();
}

void Hivemind::recvLoop() {
    while (m_recvRunning) {
        Message msg;
        if (!transport->receive(msg)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        hivemind::Packet pkt;
        if (hivemind::Packet::deserialize(
                reinterpret_cast<const uint8_t*>(msg.message.data()),
                msg.message.size(), pkt)) {
            processIncomingPacket(pkt, msg.sender_ip, msg.sender_port);
        } else {
            if (m_messageCallback) {
                m_messageCallback(msg.sender_ip + ":" + std::to_string(msg.sender_port),
                                  msg.message);
            }
        }
    }
}

bool Hivemind::sendToIp(const std::string& ip, uint16_t port, const std::string& message) {
    hivemind::Packet pkt;
    pkt.type = hivemind::MsgType::MESSAGE;
    pkt.payload.assign(message.begin(), message.end());
    auto data = pkt.serialize();
    return transport->sendWithHolePunch(ip, port, std::string(data.begin(), data.end()));
}

bool Hivemind::sendToUser(const std::string& username, const std::string& message) {
    SimpleUser user;
    if (!findUser(username, user)) {
        return resolveUsername(username, user) && sendToIp(user.ip, user.port, message);
    }
    return sendToIp(user.ip, user.port, message);
}

bool Hivemind::receive(std::string& ip, uint16_t& port, std::string& message) {
    Message msg;
    if (!transport->receive(msg)) return false;
    ip = msg.sender_ip;
    port = msg.sender_port;
    hivemind::Packet pkt;
    if (hivemind::Packet::deserialize(
            reinterpret_cast<const uint8_t*>(msg.message.data()),
            msg.message.size(), pkt)) {
        if (pkt.type == hivemind::MsgType::MESSAGE) {
            message = std::string(pkt.payload.begin(), pkt.payload.end());
        } else {
            message = "[SYSTEM] Received packet type " + std::to_string(static_cast<int>(pkt.type));
        }
    } else {
        message = msg.message;
    }
    return true;
}

bool Hivemind::registerName(const std::string& name) {
    std::cout << "[Hivemind] registerName() called with: " << name << std::endl;
    
    std::string ipToUse = m_myPublicIP.empty() ? m_myLocalIP : m_myPublicIP;
    if (ipToUse.empty()) ipToUse = "127.0.0.1";
    
    std::cout << "[Hivemind] Will use IP: " << ipToUse << ", Port: " << m_myPort << std::endl;
    
    // Сохраняем локально
    std::cout << "[Hivemind] Saving to local registry..." << std::endl;
    registry->addUser("", name, m_myNodeIdStr, ipToUse, m_myPort);
    m_myUsername = name;
    std::cout << "[Hivemind] Local save OK" << std::endl;
    
    // Если маяк настроен — отправляем регистрацию на маяк (асинхронно)
    if (beaconClient->isConfigured()) {
        std::cout << "[Hivemind] Beacon configured, sending registration..." << std::endl;
        hivemind::NodeInfo info{};
        memcpy(info.nodeId, m_myNodeId, 20);
        strncpy(info.username, name.c_str(), 31);
        info.username[31] = '\0';
        strncpy(info.publicIp, ipToUse.c_str(), 15);
        info.publicIp[15] = '\0';
        strncpy(info.localIp, m_myLocalIP.c_str(), 15);
        info.localIp[15] = '\0';
        info.port = m_myPort;
        info.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        bool sent = beaconClient->registerUsername(name, info,
            [](bool success, const hivemind::NodeInfo& nodeInfo) {
                if (success) {
                    std::cout << "\n\x1b[32m[Hivemind] ✓ Registered on beacon!\x1b[0m" << std::endl;
                } else {
                    std::cerr << "\n\x1b[31m[Hivemind] ✗ Beacon registration failed (name taken?)\x1b[0m" << std::endl;
                }
                std::cout << "> " << std::flush;
            });
        
        if (!sent) {
            std::cerr << "[Hivemind] ✗ Failed to send registration packet" << std::endl;
            return false;
        }
        
        std::cout << "[Hivemind] ✓ Registration packet sent to beacon!" << std::endl;
        std::cout << "[Hivemind] Waiting for beacon response (check /pack in 2-3 sec)..." << std::endl;
        return true;
    }
    
    std::cout << "[Hivemind] No beacon configured, local only." << std::endl;
    return true;
}

bool Hivemind::findUser(const std::string& name, SimpleUser& user) {
    return registry->findUser(name, user);
}

void Hivemind::syncWithNetwork() {
    registry->load();
    if (beaconClient->isConfigured()) {
        requestBootstrap();
    }
}

void Hivemind::startAutoSync(int intervalSeconds) {
    m_syncInterval = intervalSeconds;
    m_syncRunning = true;
    m_syncThread = std::thread(&Hivemind::syncLoop, this);
}

void Hivemind::stopAutoSync() {
    m_syncRunning = false;
}

void Hivemind::syncLoop() {
    while (m_syncRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(m_syncInterval));
        if (!m_syncRunning) break;
        syncWithNetwork();
    }
}

void Hivemind::enableHolePunching(bool enable) {
    m_holePunchingEnabled = enable;
    transport->enableHolePunching(enable);
}

void Hivemind::setBeaconAddress(const std::string& ip, uint16_t port) {
    beaconClient->setBeaconAddress(ip, port);
    std::cout << "[Hivemind] Beacon set: " << ip << ":" << port << std::endl;
}

// === НОВЫЕ МЕТОДЫ ===

bool Hivemind::requestUsersPack() {
    if (!beaconClient->isConfigured()) {
        std::cerr << "[Hivemind] Beacon not configured! Use setBeaconAddress() first." << std::endl;
        return false;
    }
    
    std::cout << "[Hivemind] =========================================" << std::endl;
    std::cout << "[Hivemind] Requesting users.pack from beacon..." << std::endl;
    std::cout << "[Hivemind] Beacon: " << beaconClient->getBeaconIp() << ":" << beaconClient->getBeaconPort() << std::endl;
    std::cout << "[Hivemind] My NodeID: " << m_myNodeIdStr << std::endl;
    std::cout << "[Hivemind] My Public IP: " << m_myPublicIP << std::endl;
    std::cout << "[Hivemind] My Port: " << m_myPort << std::endl;
    
    auto data = hivemind::createRequestUsersPackPacket(m_myNodeIdStr);
    std::cout << "[Hivemind] Packet size: " << data.size() << " bytes" << std::endl;
    
    bool sent = transport->sendTo(beaconClient->getBeaconIp(), beaconClient->getBeaconPort(),
                                   std::string(data.begin(), data.end()));
    
    if (sent) {
        std::cout << "[Hivemind] ✓ Users pack request SENT to beacon" << std::endl;
        std::cout << "[Hivemind] Waiting for response... (timeout 5s)" << std::endl;
    } else {
        std::cerr << "[Hivemind] ✗ FAILED to send request!" << std::endl;
    }
    std::cout << "[Hivemind] =========================================" << std::endl;
    return sent;
}

bool Hivemind::isInUsersPack() const {
    std::lock_guard<std::mutex> lock(m_networkUsersMutex);
    for (const auto& user : m_networkUsers) {
        if (user.nodeId == m_myNodeIdStr) {
            return true;
        }
    }
    return false;
}

std::vector<SimpleUser> Hivemind::getNetworkUsers() const {
    std::lock_guard<std::mutex> lock(m_networkUsersMutex);
    return m_networkUsers;
}

bool Hivemind::sendViaRelay(const std::string& toUsername, const std::string& message) {
    if (!beaconClient->isConfigured()) {
        std::cerr << "[Hivemind] Beacon not configured!" << std::endl;
        return false;
    }
    
    // Находим получателя в сети
    SimpleUser target;
    {
        std::lock_guard<std::mutex> lock(m_networkUsersMutex);
        for (const auto& user : m_networkUsers) {
            if (user.name == toUsername) {
                target = user;
                break;
            }
        }
    }
    
    if (target.name.empty()) {
        std::cerr << "[Hivemind] User not found in network: " << toUsername << std::endl;
        return false;
    }
    
    // Отправляем сообщение через маяк
    auto data = hivemind::createRelayMessagePacket(m_myNodeIdStr, target.nodeId, message);
    return transport->sendTo(beaconClient->getBeaconIp(), beaconClient->getBeaconPort(),
                              std::string(data.begin(), data.end()));
}

bool Hivemind::isBeaconConfigured() const {
    return beaconClient->isConfigured();
}

// === УСТАРЕВШИЕ МЕТОДЫ ===

bool Hivemind::registerUsername(const std::string& username) {
    return registerName(username);
}

bool Hivemind::resolveUsername(const std::string& username, SimpleUser& user) {
    if (!beaconClient->isConfigured()) {
        return false;
    }
    bool result = false;
    bool completed = false;
    beaconClient->resolveUsername(username,
        [&result, &completed, &user](bool success, const hivemind::NodeInfo& info) {
            result = success;
            completed = true;
            if (success) {
                user.ip = info.publicIp;
                user.port = info.port;
                std::cout << "[Hivemind] Resolved @" << info.username
                          << " -> " << user.ip << ":" << user.port << std::endl;
            }
        });
    int timeout = 50;
    while (!completed && timeout-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return completed && result;
}

bool Hivemind::isBeaconConnected() const {
    return beaconClient->isConfigured();
}

bool Hivemind::requestBootstrap() {
    if (!beaconClient->isConfigured()) return false;
    bool completed = false;
    beaconClient->requestBootstrap(m_myNodeId,
        [&completed](const std::vector<hivemind::NodeInfo>& nodes) {
            std::cout << "[Hivemind] Received " << nodes.size() << " bootstrap nodes" << std::endl;
            for (const auto& node : nodes) {
                std::cout << "  - Node: " << node.publicIp << ":" << node.port
                          << " (@" << node.username << ")" << std::endl;
            }
            completed = true;
        });
    int timeout = 50;
    while (!completed && timeout-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return completed;
}

void Hivemind::setMessageCallback(MessageCallback cb) {
    m_messageCallback = cb;
}

void Hivemind::onHolePunchDetected(const std::string& ip, uint16_t port) {
    std::cout << "[Hivemind] Hole punch successful with " << ip << ":" << port << std::endl;
}

void Hivemind::processIncomingPacket(const hivemind::Packet& pkt,
                                      const std::string& fromIp, uint16_t fromPort) {
    beaconClient->handlePacket(pkt, fromIp, fromPort);
    
    switch (pkt.type) {
        case hivemind::MsgType::MESSAGE: {
            std::string msg(pkt.payload.begin(), pkt.payload.end());
            if (m_messageCallback) {
                m_messageCallback(fromIp + ":" + std::to_string(fromPort), msg);
            }
            break;
        }
        case hivemind::MsgType::PING: {
            hivemind::Packet response;
            response.type = hivemind::MsgType::PONG;
            auto data = response.serialize();
            transport->sendTo(fromIp, fromPort, std::string(data.begin(), data.end()));
            break;
        }
        
        // === ОБРАБОТКА USERS_PACK_RESPONSE ===
        case hivemind::MsgType::USERS_PACK_RESPONSE: {
            std::cout << "[Hivemind] Received users.pack from beacon" << std::endl;
            std::lock_guard<std::mutex> lock(m_networkUsersMutex);
            m_networkUsers.clear();
            
            if (pkt.payload.size() < 1) break;
            uint8_t count = pkt.payload[0];
            size_t offset = 1;
            
            for (int i = 0; i < count && offset + 2 <= pkt.payload.size(); ++i) {
                uint16_t len = (pkt.payload[offset] << 8) | pkt.payload[offset + 1];
                offset += 2;
                if (offset + len > pkt.payload.size()) break;
                
                std::string line(pkt.payload.begin() + offset, pkt.payload.begin() + offset + len);
                offset += len;
                
                // Парсим строку: BEACON_IP|USERNAME|NODE_ID|PUBLIC_IP|PORT
                std::stringstream ss(line);
                std::string beaconIp, name, nodeId, ip, portStr;
                if (std::getline(ss, beaconIp, '|') &&
                    std::getline(ss, name, '|') &&
                    std::getline(ss, nodeId, '|') &&
                    std::getline(ss, ip, '|') &&
                    std::getline(ss, portStr)) {
                    SimpleUser user;
                    user.beaconIp = beaconIp;
                    user.name = name;
                    user.nodeId = nodeId;
                    user.ip = ip;
                    user.port = static_cast<uint16_t>(std::stoi(portStr));
                    m_networkUsers.push_back(user);
                }
            }
            
            m_usersPackReceived = true;
            std::cout << "[Hivemind] Loaded " << m_networkUsers.size() << " users from network" << std::endl;
            
            // Проверяем, есть ли мы в списке
            bool foundSelf = false;
            for (const auto& user : m_networkUsers) {
                if (user.nodeId == m_myNodeIdStr) {
                    foundSelf = true;
                    break;
                }
            }
            if (foundSelf) {
                std::cout << "[Hivemind] ✓ You are registered in the network!" << std::endl;
            } else {
                std::cout << "[Hivemind] ⚠ You are NOT in the network yet. Register with /reg <name>" << std::endl;
            }
            break;
        }
        
        // === ОБРАБОТКА РЕТРАНСЛИРОВАННОГО СООБЩЕНИЯ ===
        case hivemind::MsgType::RELAY_MESSAGE: {
            if (pkt.payload.size() < 80) break; // 40 + 40 bytes node IDs minimum
            
            std::string fromNodeId(pkt.payload.begin(), pkt.payload.begin() + 40);
            std::string toNodeId(pkt.payload.begin() + 40, pkt.payload.begin() + 80);
            
            if (toNodeId != m_myNodeIdStr) {
                // Не нам — игнорируем
                break;
            }
            
            uint16_t msgLen = (pkt.payload[80] << 8) | pkt.payload[81];
            std::string message(pkt.payload.begin() + 82, pkt.payload.begin() + 82 + msgLen);
            
            // Находим имя отправителя
            std::string fromName = fromNodeId.substr(0, 8);
            {
                std::lock_guard<std::mutex> lock(m_networkUsersMutex);
                for (const auto& user : m_networkUsers) {
                    if (user.nodeId == fromNodeId) {
                        fromName = user.name;
                        break;
                    }
                }
            }
            
            std::cout << "\n\x1b[35m[Relay from " << fromName << " (" << fromNodeId.substr(0, 8) << "...)]:\x1b[0m " 
                      << message << std::endl;
            std::cout << "> " << std::flush;
            
            if (m_messageCallback) {
                m_messageCallback("relay:" + fromName, message);
            }
            break;
        }
        
        default:
            break;
    }
}

