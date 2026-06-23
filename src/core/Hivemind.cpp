// src/core/Hivemind.cpp
#include "Hivemind.hpp"
#include <cstring>

Hivemind::Hivemind() : transport(std::make_unique<Transport>()) {}

Hivemind::~Hivemind() {
    stop();
}

bool Hivemind::start(uint16_t port) {
    return transport->start(port);
}

void Hivemind::stop() {
    transport->stop();
}

bool Hivemind::sendToIp(const std::string& ip, uint16_t port, const std::string& message) {
    return transport->sendTo(ip, port, message);
}

bool Hivemind::receive(std::string& sender_ip, uint16_t& sender_port, std::string& message) {
    Message msg;
    if (!transport->receive(msg)) return false;
    
    sender_ip = msg.sender_ip;
    sender_port = msg.sender_port;
    message = msg.message;
    return true;
}