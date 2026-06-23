// src/storage/UserRegistry.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>

struct UserEntry {
    std::string name;
    std::string publicIP;
    uint16_t port;
    std::chrono::system_clock::time_point lastUpdate;
    
    std::string toString() const;
    static UserEntry fromString(const std::string& line);
    bool isExpired(int seconds = 3600) const;
};

class UserRegistry {
public:
    UserRegistry(const std::string& filePath = "all-users.pack");
    ~UserRegistry();

    bool load();
    bool save();
    
    bool registerMe(const std::string& name, const std::string& publicIP, uint16_t port);
    bool addOrUpdate(const std::string& name, const std::string& publicIP, uint16_t port);
    bool findUser(const std::string& name, UserEntry& entry) const;
    std::vector<UserEntry> getAllUsers() const;
    int cleanupExpired(int maxAgeSeconds = 3600);
    std::string getMyPublicIP() const;
    std::string toString() const;
    
private:
    std::string m_filePath;
    std::unordered_map<std::string, UserEntry> m_users;
    std::string m_myName;
    mutable std::mutex m_mutex;
};
