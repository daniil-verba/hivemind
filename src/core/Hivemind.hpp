// src/core/Hivemind.hpp
#pragma once

#include "../network/Transport.hpp"
#include <memory>

class Hivemind {
public:
    Hivemind();
    ~Hivemind();
    
    bool start(uint16_t port);
    void stop();
    
    bool sendToIp(const std::string& ip, uint16_t port, const std::string& message);
    bool receive(std::string& sender_ip, uint16_t& sender_port, std::string& message);
    
private:
    std::unique_ptr<Transport> transport;
};