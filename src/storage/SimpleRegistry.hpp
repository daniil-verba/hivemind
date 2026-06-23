// src/storage/SimpleRegistry.hpp
#pragma once

#include <string>
#include <vector>
#include <mutex>

struct SimpleUser {
    std::string name;
    std::string ip;
    uint16_t port;
};

class SimpleRegistry {
public:
    SimpleRegistry(const std::string& filePath = "users.txt");
    
    bool load();
    bool save();
    
    bool addUser(const std::string& name, const std::string& ip, uint16_t port);
    bool findUser(const std::string& name, SimpleUser& user);
    std::vector<SimpleUser> getAllUsers() const;
    void printUsers() const;
    
private:
    std::string m_filePath;
    std::vector<SimpleUser> m_users;
    mutable std::mutex m_mutex;
};