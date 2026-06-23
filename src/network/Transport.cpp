// src/network/Transport.cpp
#include "Transport.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

Transport::Transport() : sockfd(-1), running(false) {}

Transport::~Transport() {
    stop();
}

bool Transport::start(uint16_t port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return false;
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return false;
    }
    
    running = true;
    receiveThread = std::thread(&Transport::receiveLoop, this);
    
    return true;
}

void Transport::stop() {
    running = false;
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

bool Transport::sendTo(const std::string& ip, uint16_t port, const std::string& message) {
    if (sockfd < 0) return false;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    int sent = sendto(sockfd, message.c_str(), message.size(), 0,
                      (struct sockaddr*)&addr, sizeof(addr));
    
    return sent == static_cast<int>(message.size());
}

bool Transport::receive(Message& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (messageQueue.empty()) return false;
    
    msg = messageQueue.front();
    messageQueue.pop();
    return true;
}

void Transport::receiveLoop() {
    char buffer[65536];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout
        
        int ret = select(sockfd + 1, &fds, nullptr, nullptr, &tv);
        if (ret <= 0) continue;
        
        int received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr*)&from_addr, &from_len);
        if (received > 0) {
            buffer[received] = '\0';
            
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
            
            Message msg;
            msg.sender_ip = ip_str;
            msg.sender_port = ntohs(from_addr.sin_port);
            msg.message = std::string(buffer, received);
            
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(msg);
        }
    }
}