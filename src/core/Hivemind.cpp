// src/core/Hivemind.cpp
#include "Hivemind.hpp"
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

Hivemind::Hivemind() 
    : transport(std::make_unique<Transport>())
    , registry(std::make_unique<UserRegistry>("all-users.pack"))
{
    registry->load();
    
    transport->setHolePunchCallback([this](const std::string& ip, uint16_t port) {
        onHolePunchDetected(ip, port);
    });
}

Hivemind::~Hivemind() {
    stop();
}

bool Hivemind::start(uint16_t port) {
    m_myPort = port;
    
    if (!transport->start(port)) {
        return false;
    }
    
    // Определяем публичный IP через STUN
    m_myPublicIP = StunClient::getPublicIP();
    
    if (m_myPublicIP.empty()) {
        std::cout << "[Hivemind] Warning: Could not determine public IP" << std::endl;
    } else {
        std::cout << "[Hivemind] Public IP: " << m_myPublicIP << ":" << port << std::endl;
    }
    
    syncWithNetwork();
    startAutoSync(600);
    
    return true;
}

void Hivemind::stop() {
    stopAutoSync();
    transport->stop();
}

bool Hivemind::sendToIp(const std::string& ip, uint16_t port, const std::string& message) {
    return transport->sendWithHolePunch(ip, port, message);
}

bool Hivemind::receive(std::string& sender_ip, uint16_t& sender_port, std::string& message) {
    Message msg;
    if (!transport->receive(msg)) return false;
    
    sender_ip = msg.sender_ip;
    sender_port = msg.sender_port;
    message = msg.message;
    return true;
}

std::string Hivemind::getMyPublicIP() {
    // Если уже есть — возвращаем
    if (!m_myPublicIP.empty()) {
        return m_myPublicIP;
    }
    
    // Получаем через STUN
    m_myPublicIP = StunClient::getPublicIP();
    return m_myPublicIP;
}

bool Hivemind::registerName(const std::string& name) {
    m_myName = name;
    
    // Обновляем IP перед регистрацией
    m_myPublicIP = getMyPublicIP();
    
    if (m_myPublicIP.empty()) {
        std::cerr << "[Hivemind] Cannot register: no public IP" << std::endl;
        return false;
    }
    
    return registry->registerMe(name, m_myPublicIP, m_myPort);
}

bool Hivemind::findUser(const std::string& name, UserEntry& entry) {
    return registry->findUser(name, entry);
}

bool Hivemind::sendToUser(const std::string& name, const std::string& message) {
    UserEntry entry;
    if (!findUser(name, entry)) {
        std::cerr << "[Hivemind] User not found: " << name << std::endl;
        return false;
    }
    
    return sendToIp(entry.publicIP, entry.port, message);
}

void Hivemind::syncWithNetwork() {
    // Обновляем IP перед синхронизацией
    m_myPublicIP = getMyPublicIP();
    
    if (!m_myName.empty() && !m_myPublicIP.empty()) {
        registry->registerMe(m_myName, m_myPublicIP, m_myPort);
    }
    
    registry->cleanupExpired();
    
    std::cout << "[Hivemind] Sync completed. " 
              << registry->getAllUsers().size() << " users active" << std::endl;
}

void Hivemind::startAutoSync(int intervalSeconds) {
    if (m_autoSyncRunning) return;
    
    m_autoSyncRunning = true;
    m_autoSyncThread = std::thread([this, intervalSeconds]() {
        while (m_autoSyncRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            
            // Проверяем, не изменился ли IP
            std::string newIP = StunClient::getPublicIP();
            if (!newIP.empty() && newIP != m_myPublicIP) {
                m_myPublicIP = newIP;
                if (!m_myName.empty()) {
                    registry->registerMe(m_myName, m_myPublicIP, m_myPort);
                }
            }
            
            syncWithNetwork();
        }
    });
}

void Hivemind::stopAutoSync() {
    m_autoSyncRunning = false;
    if (m_autoSyncThread.joinable()) {
        m_autoSyncThread.join();
    }
}

void Hivemind::enableHolePunching(bool enable) {
    m_holePunchingEnabled = enable;
    transport->enableHolePunching(enable);
}

void Hivemind::setHolePunchCallback(std::function<void(const std::string&, uint16_t)> cb) {
    m_holePunchCallback = cb;
}

void Hivemind::onHolePunchDetected(const std::string& ip, uint16_t port) {
    std::cout << "[Hivemind] Hole punch detected with " << ip << ":" << port << std::endl;
    if (m_holePunchCallback) {
        m_holePunchCallback(ip, port);
    }
}