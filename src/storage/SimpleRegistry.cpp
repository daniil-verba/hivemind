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
        std::cout << "[SimpleRegistry] No users.txt, creating new..." << std::endl;
        m_users.clear();
        return true;
    }
    
    m_users.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::stringstream ss(line);
        std::string name, ip, portStr;
        
        if (std::getline(ss, name, '|') &&
            std::getline(ss, ip, '|') &&
            std::getline(ss, portStr)) {
            SimpleUser user;
            user.name = name;
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
    //std::lock_guard<std::mutex> lock(m_mutex);
    
    std::cout << "[SimpleRegistry] save() called, path: " << m_filePath << std::endl;
    std::cout << "[SimpleRegistry] Users to save: " << m_users.size() << std::endl;
    
    std::ofstream file(m_filePath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[SimpleRegistry] ❌ Failed to open " << m_filePath << " for writing" << std::endl;
        std::cerr << "[SimpleRegistry] Error: " << strerror(errno) << std::endl;
        return false;
    }
    
    file << "# users.txt - Simple P2P Registry\n";
    file << "# Format: NAME|IP|PORT\n";
    
    for (const auto& user : m_users) {
        file << user.name << "|" << user.ip << "|" << user.port << "\n";
        std::cout << "[SimpleRegistry] Writing: " << user.name << "|" << user.ip << "|" << user.port << std::endl;
    }
    
    file.close();
    
    if (file.fail()) {
        std::cerr << "[SimpleRegistry] ❌ Write failed for: " << m_filePath << std::endl;
        return false;
    }
    
    std::cout << "[SimpleRegistry] ✅ Saved " << m_users.size() << " users to " << m_filePath << std::endl;
    return true;
}

bool SimpleRegistry::addUser(const std::string& name, const std::string& ip, uint16_t port) {
    std::cout << "[SimpleRegistry] addUser() called: " << name << " -> " << ip << ":" << port << std::endl;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Проверяем, есть ли уже такой пользователь
    for (auto& user : m_users) {
        if (user.name == name) {
            std::cout << "[SimpleRegistry] User exists, updating..." << std::endl;
            user.ip = ip;
            user.port = port;
            bool result = save();
            std::cout << "[SimpleRegistry] Update result: " << result << std::endl;
            return result;
        }
    }
    
    SimpleUser user;
    user.name = name;
    user.ip = ip;
    user.port = port;
    m_users.push_back(user);
    
    std::cout << "[SimpleRegistry] New user added, saving..." << std::endl;
    bool result = save();
    std::cout << "[SimpleRegistry] Save result: " << result << std::endl;
    return result;
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

std::vector<SimpleUser> SimpleRegistry::getAllUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_users;
}

void SimpleRegistry::printUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::cout << "[SimpleRegistry] Users (" << m_users.size() << "):" << std::endl;
    for (const auto& user : m_users) {
        std::cout << "  " << user.name << " -> " << user.ip << ":" << user.port << std::endl;
    }
}

// src/storage/SimpleRegistry.cpp


