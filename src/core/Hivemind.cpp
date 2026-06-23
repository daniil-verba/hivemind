// src/core/Hivemind.cpp
#include "Hivemind.hpp"
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

Hivemind::Hivemind() 
    : transport(std::make_unique<Transport>())
    , registry(std::make_unique<SimpleRegistry>("users.txt"))
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
    if (!m_myPublicIP.empty()) {
        return m_myPublicIP;
    }
    
    m_myPublicIP = StunClient::getPublicIP();
    return m_myPublicIP;
}

// src/core/Hivemind.cpp

bool Hivemind::registerName(const std::string& name) {
    std::cout << "[Hivemind] registerName() called with: " << name << std::endl;
    
    m_myName = name;
    
    m_myPublicIP = getMyPublicIP();
    std::cout << "[Hivemind] Public IP: " << m_myPublicIP << std::endl;
    
    if (m_myPublicIP.empty()) {
        std::cerr << "[Hivemind] Cannot register: no public IP" << std::endl;
        return false;
    }
    
    std::cout << "[Hivemind] Calling registry->addUser()..." << std::endl;
    bool result = registry->addUser(name, m_myPublicIP, m_myPort);
    std::cout << "[Hivemind] registry->addUser() returned: " << result << std::endl;
    
    if (result) {
        std::cout << "[Hivemind] ✅ Registered: " << name << " -> " 
                  << m_myPublicIP << ":" << m_myPort << std::endl;
        registry->printUsers();
    } else {
        std::cerr << "[Hivemind] ❌ Registration failed!" << std::endl;
    }
    
    return result;
}

bool Hivemind::findUser(const std::string& name, SimpleUser& user) {
    return registry->findUser(name, user);
}

bool Hivemind::sendToUser(const std::string& name, const std::string& message) {
    SimpleUser user;
    if (!findUser(name, user)) {
        std::cerr << "[Hivemind] User not found: " << name << std::endl;
        return false;
    }
    
    return sendToIp(user.ip, user.port, message);
}

void Hivemind::syncWithNetwork() {
    m_myPublicIP = getMyPublicIP();
    
    if (!m_myName.empty() && !m_myPublicIP.empty()) {
        registry->addUser(m_myName, m_myPublicIP, m_myPort);
    }
    
    registry->printUsers();
}

void Hivemind::startAutoSync(int intervalSeconds) {
    if (m_autoSyncRunning) return;
    
    m_autoSyncRunning = true;
    m_autoSyncThread = std::thread([this, intervalSeconds]() {
        while (m_autoSyncRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            
            std::string newIP = StunClient::getPublicIP();
            if (!newIP.empty() && newIP != m_myPublicIP) {
                m_myPublicIP = newIP;
                if (!m_myName.empty()) {
                    registry->addUser(m_myName, m_myPublicIP, m_myPort);
                }
            }
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