// src/network/Transport.hpp
#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

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
    
private:
    int sockfd;
    std::atomic<bool> running;
    std::thread receiveThread;
    std::queue<Message> messageQueue;
    std::mutex queueMutex;
    
    void receiveLoop();
};