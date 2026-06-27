// src/storage/SimpleRegistry.cpp
#include "SimpleRegistry.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <cstring>

SimpleRegistry::SimpleRegistry(const std::string& filePath) : m_filePath(filePath) {}

bool SimpleRegistry::load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        std::cout << "[SimpleRegistry] No " << m_filePath << ", creating new..." << std::endl;
        m_users.clear();
        return true;
    }
    
    m_users.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
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
            m_users.push_back(user);
        }
    }
    
    file.close();
    std::cout << "[SimpleRegistry] Loaded " << m_users.size() << " users" << std::endl;
    return true;
}

bool SimpleRegistry::save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ofstream file(m_filePath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[SimpleRegistry] Failed to open " << m_filePath << " for writing: " << strerror(errno) << std::endl;
        return false;
    }
    
    file << "# users.pack - Hivemind P2P Registry\\n";
    file << "# Format: BEACON_IP|USERNAME|NODE_ID|PUBLIC_IP|PORT\\n";
    
    for (const auto& user : m_users) {
        file << user.beaconIp << "|" << user.name << "|" << user.nodeId << "|" 
             << user.ip << "|" << user.port << "\\n";
    }
    
    file.close();
    std::cout << "[SimpleRegistry] Saved " << m_users.size() << " users to " << m_filePath << std::endl;
    return true;
}

bool SimpleRegistry::addUser(const std::string& beaconIp, const std::string& name, 
                              const std::string& nodeId, const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Проверяем, есть ли уже такой пользователь
    for (auto& user : m_users) {
        if (user.name == name || user.nodeId == nodeId) {
            user.beaconIp = beaconIp;
            user.ip = ip;
            user.port = port;
            return save();
        }
    }
    
    SimpleUser user;
    user.beaconIp = beaconIp;
    user.name = name;
    user.nodeId = nodeId;
    user.ip = ip;
    user.port = port;
    m_users.push_back(user);
    
    return save();
}

bool SimpleRegistry::findUser(const std::string& name, SimpleUser& user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& u : m_users) {
        if (u.name == name) {
            user = u;
            return true;
        }
    }
    return false;
}

bool SimpleRegistry::findUserByNodeId(const std::string& nodeId, SimpleUser& user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& u : m_users) {
        if (u.nodeId == nodeId) {
            user = u;
            return true;
        }
    }
    return false;
}

std::vector<SimpleUser> SimpleRegistry::getAllUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_users;
}

void SimpleRegistry::printUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "[SimpleRegistry] Users (" << m_users.size() << "):" << std::endl;
    for (const auto& user : m_users) {
        std::cout << "  [" << user.beaconIp << "] " << user.name 
                  << " (" << user.nodeId << ") -> " << user.ip << ":" << user.port << std::endl;
    }
}

void SimpleRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_users.clear();
}