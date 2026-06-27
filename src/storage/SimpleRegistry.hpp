// include/hivemind/SimpleRegistry.hpp
#pragma once
#include <string>
#include <vector>
#include <mutex>

struct SimpleUser {
    std::string beaconIp;    // IP маяка
    std::string name;        // Имя пользователя
    std::string nodeId;      // Уникальный ID (40 hex символов)
    std::string ip;          // Публичный IP
    uint16_t port;           // Порт
};

class SimpleRegistry {
public:
    explicit SimpleRegistry(const std::string& filePath);
    
    bool load();
    bool save();
    bool addUser(const std::string& beaconIp, const std::string& name, 
                 const std::string& nodeId, const std::string& ip, uint16_t port);
    bool findUser(const std::string& name, SimpleUser& user);
    bool findUserByNodeId(const std::string& nodeId, SimpleUser& user);
    std::vector<SimpleUser> getAllUsers() const;
    void printUsers() const;
    void clear();
    
private:
    std::string m_filePath;
    std::vector<SimpleUser> m_users;
    mutable std::mutex m_mutex;
};