// src/network/Transport.cpp
#include "Transport.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>

Transport::Transport() : sockfd(-1), running(false) {}

Transport::~Transport() {
    stop();
}

bool Transport::start(uint16_t port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return false;
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return false;
    }
    
    running = true;
    receiveThread = std::thread(&Transport::receiveLoop, this);
    
    return true;
}

void Transport::stop() {
    running = false;
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

bool Transport::sendTo(const std::string& ip, uint16_t port, const std::string& message) {
    if (sockfd < 0) {
        std::cerr << "[Transport] sendTo failed: socket not initialized" << std::endl;
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "[Transport] sendTo failed: invalid IP " << ip << std::endl;
        return false;
    }
    
    int sent = sendto(sockfd, message.c_str(), message.size(), 0,
                      (struct sockaddr*)&addr, sizeof(addr));
    
    if (sent == static_cast<int>(message.size())) {
        std::cout << "[Transport] Sent " << sent << " bytes to " << ip << ":" << port << std::endl;
        return true;
    } else {
        std::cerr << "[Transport] sendTo failed: sent " << sent << " of " << message.size() 
                  << " bytes to " << ip << ":" << port << std::endl;
        return false;
    }
}

bool Transport::receive(Message& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (messageQueue.empty()) return false;
    
    msg = messageQueue.front();
    messageQueue.pop();
    return true;
}

// ============================================================
// Rate Limiting
// ============================================================

bool Transport::isRateLimited(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_rateLimitMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto it = m_rateLimitMap.find(ip);
    
    if (it == m_rateLimitMap.end()) {
        m_rateLimitMap[ip] = {now, 1};
        return false;
    }
    
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastPacket);
    
    if (diff.count() < 1000) {  // 1 секунда
        it->second.packetCount++;
        if (it->second.packetCount > m_maxPacketsPerSecond) {
            return true;  // слишком много пакетов
        }
    } else {
        it->second.packetCount = 1;
        it->second.lastPacket = now;
    }
    
    return false;
}

void Transport::setRateLimit(int packetsPerSecond) {
    m_maxPacketsPerSecond = packetsPerSecond;
}

// ============================================================
// Hole Punching
// ============================================================

void Transport::enableHolePunching(bool enable) {
    m_holePunchingEnabled = enable;
}

bool Transport::isFirstContact(const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_contactsMutex);
    std::string key = ip + ":" + std::to_string(port);
    return m_knownContacts.find(key) == m_knownContacts.end();
}

void Transport::markContact(const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_contactsMutex);
    std::string key = ip + ":" + std::to_string(port);
    m_knownContacts.insert(key);
}

bool Transport::sendWithHolePunch(const std::string& ip, uint16_t port, const std::string& message) {
    if (!m_holePunchingEnabled) {
        return sendTo(ip, port, message);
    }
    
    // Проверяем Rate Limiting
    if (isRateLimited(ip)) {
        std::cout << "[Transport] Rate limited: " << ip << std::endl;
        return false;
    }
    
    // Отправляем сообщение
    bool sent = sendTo(ip, port, message);
    
    // Если это первый контакт — делаем Hole Punching
    if (isFirstContact(ip, port)) {
        std::cout << "[Transport] First contact with " << ip << ":" << port 
                  << " — starting hole punching" << std::endl;
        
        // Отправляем несколько пакетов для пробития NAT
        for (int i = 0; i < 5; i++) {
            sendTo(ip, port, "PING");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        markContact(ip, port);
        
        // Вызываем колбэк (для уведомления UI)
        if (m_holePunchCallback) {
            m_holePunchCallback(ip, port);
        }
    }
    
    return sent;
}

void Transport::setHolePunchCallback(std::function<void(const std::string&, uint16_t)> cb) {
    m_holePunchCallback = cb;
}

// ============================================================
// Receive Loop
// ============================================================

void Transport::receiveLoop() {
    char buffer[65536];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms timeout
        
        int ret = select(sockfd + 1, &fds, nullptr, nullptr, &tv);
        if (ret <= 0) continue;
        
        int received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr*)&from_addr, &from_len);
        if (received > 0) {
            buffer[received] = '\0';
            
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
            uint16_t port = ntohs(from_addr.sin_port);
            
            // Проверяем Rate Limiting
            if (isRateLimited(ip_str)) {
                continue;  // игнорируем слишком частые пакеты
            }
            
            // Если это PING — просто игнорируем (или можно ответить PONG)
            if (std::string(buffer) == "PING") {
                // Отвечаем PONG для подтверждения связи
                sendTo(ip_str, port, "PONG");
                continue;
            }
            
            // Если это PONG — помечаем контакт
            if (std::string(buffer) == "PONG") {
                markContact(ip_str, port);
                continue;
            }
            
            // Обычное сообщение
            Message msg;
            msg.sender_ip = ip_str;
            msg.sender_port = port;
            msg.message = std::string(buffer, received);
            
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(msg);
        }
    }
}
