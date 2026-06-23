// src/storage/UserRegistry.cpp
#include "UserRegistry.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

// ============================================================
// UserEntry
// ============================================================

std::string UserEntry::toString() const {
    auto time_t = std::chrono::system_clock::to_time_t(lastUpdate);
    std::tm* tm = std::gmtime(&time_t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm);
    return name + "|" + publicIP + "|" + std::to_string(port) + "|" + buf;
}

UserEntry UserEntry::fromString(const std::string& line) {
    UserEntry entry;
    std::stringstream ss(line);
    std::string token;
    
    std::getline(ss, entry.name, '|');
    std::getline(ss, entry.publicIP, '|');
    std::getline(ss, token, '|');
    entry.port = static_cast<uint16_t>(std::stoi(token));
    std::getline(ss, token, '|');
    
    // Парсим время (упрощённо — используем текущее)
    entry.lastUpdate = std::chrono::system_clock::now();
    
    return entry;
}

bool UserEntry::isExpired(int seconds) const {
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
    return diff.count() > seconds;
}

// ============================================================
// UserRegistry
// ============================================================

UserRegistry::UserRegistry(const std::string& filePath) : m_filePath(filePath) {}

UserRegistry::~UserRegistry() {
    save();
}

bool UserRegistry::load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        std::cout << "[UserRegistry] No existing file, starting fresh" << std::endl;
        return true;
    }
    
    m_users.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        UserEntry entry = UserEntry::fromString(line);
        m_users[entry.name] = entry;
    }
    
    file.close();
    std::cout << "[UserRegistry] Loaded " << m_users.size() << " users" << std::endl;
    return true;
}

bool UserRegistry::save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ofstream file(m_filePath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[UserRegistry] Failed to save to " << m_filePath << std::endl;
        return false;
    }
    
    file << "# all-users.pack - P2P User Registry\n";
    file << "# Format: NAME|PUBLIC_IP|PORT|LAST_UPDATE\n";
    
    for (const auto& pair : m_users) {
        file << pair.second.toString() << "\n";
    }
    
    file.close();
    return true;
}

bool UserRegistry::registerMe(const std::string& name, const std::string& publicIP, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_myName = name;
    UserEntry entry;
    entry.name = name;
    entry.publicIP = publicIP;
    entry.port = port;
    entry.lastUpdate = std::chrono::system_clock::now();
    
    m_users[name] = entry;
    save();
    
    std::cout << "[UserRegistry] Registered: " << name << " -> " << publicIP << ":" << port << std::endl;
    return true;
}

bool UserRegistry::addOrUpdate(const std::string& name, const std::string& publicIP, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_users.find(name);
    if (it != m_users.end()) {
        if (it->second.publicIP == publicIP && it->second.port == port) {
            return true;
        }
    }
    
    UserEntry entry;
    entry.name = name;
    entry.publicIP = publicIP;
    entry.port = port;
    entry.lastUpdate = std::chrono::system_clock::now();
    
    m_users[name] = entry;
    save();
    
    std::cout << "[UserRegistry] Updated: " << name << " -> " << publicIP << ":" << port << std::endl;
    return true;
}

bool UserRegistry::findUser(const std::string& name, UserEntry& entry) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_users.find(name);
    if (it == m_users.end()) return false;
    
    if (it->second.isExpired()) {
        return false;
    }
    
    entry = it->second;
    return true;
}

std::vector<UserEntry> UserRegistry::getAllUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<UserEntry> result;
    for (const auto& pair : m_users) {
        if (!pair.second.isExpired()) {
            result.push_back(pair.second);
        }
    }
    return result;
}

int UserRegistry::cleanupExpired(int maxAgeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int removed = 0;
    auto it = m_users.begin();
    while (it != m_users.end()) {
        if (it->second.isExpired(maxAgeSeconds)) {
            it = m_users.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    if (removed > 0) {
        save();
        std::cout << "[UserRegistry] Cleaned up " << removed << " expired entries" << std::endl;
    }
    
    return removed;
}

std::string UserRegistry::getMyPublicIP() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_users.find(m_myName);
    if (it == m_users.end()) return "";
    
    return it->second.publicIP;
}

std::string UserRegistry::toString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string result = "# all-users.pack - P2P User Registry\n";
    result += "# Format: NAME|PUBLIC_IP|PORT|LAST_UPDATE\n";
    
    for (const auto& pair : m_users) {
        result += pair.second.toString() + "\n";
    }
    
    return result;
}
