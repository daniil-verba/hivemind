// src/network/Transport.hpp
#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <functional>

struct Message {
    std::string sender_ip;
    uint16_t sender_port;
    std::string message;
};

class Transport {
public:
    Transport();
    ~Transport();
    
    bool start(uint16_t port);
    void stop();
    
    bool sendTo(const std::string& ip, uint16_t port, const std::string& message);
    bool receive(Message& msg);
    
    // ======= Hole Punching =======
    void enableHolePunching(bool enable);
    bool sendWithHolePunch(const std::string& ip, uint16_t port, const std::string& message);
    void setHolePunchCallback(std::function<void(const std::string&, uint16_t)> cb);
    
    // ======= Rate Limiting =======
    void setRateLimit(int packetsPerSecond);
    
private:
    void receiveLoop();
    bool isRateLimited(const std::string& ip);
    bool isFirstContact(const std::string& ip, uint16_t port);
    void markContact(const std::string& ip, uint16_t port);
    
    int sockfd;
    std::atomic<bool> running;
    std::thread receiveThread;
    std::queue<Message> messageQueue;
    std::mutex queueMutex;
    
    // Hole Punching
    bool m_holePunchingEnabled = true;
    std::unordered_set<std::string> m_knownContacts;
    std::mutex m_contactsMutex;
    std::function<void(const std::string&, uint16_t)> m_holePunchCallback;
    
    // Rate Limiting
    int m_maxPacketsPerSecond = 100;
    struct RateLimitInfo {
        std::chrono::steady_clock::time_point lastPacket;
        int packetCount;
    };
    std::unordered_map<std::string, RateLimitInfo> m_rateLimitMap;
    std::mutex m_rateLimitMutex;
};
