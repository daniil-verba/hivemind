#include "hivemind/hivemind.h"
#include "core/Hivemind.hpp"
#include <cstring>
#include <string>

extern "C" {

Hivemind* hivemind_create() {
    return new Hivemind();
}

void hivemind_destroy(Hivemind* h) {
    delete h;
}

int hivemind_start(Hivemind* h, uint16_t port) {
    return h->start(port) ? 1 : 0;
}

void hivemind_stop(Hivemind* h) {
    h->stop();
}

int hivemind_send_to_ip(Hivemind* h, const char* ip, uint16_t port, const char* message) {
    return h->sendToIp(ip, port, message) ? 1 : 0;
}

int hivemind_receive(Hivemind* h, char* sender_ip, uint16_t* sender_port,
                     char* message, size_t message_size) {
    std::string ip, msg;
    uint16_t port;
    
    if (!h->receive(ip, port, msg)) return 0;
    
    strncpy(sender_ip, ip.c_str(), 15);
    sender_ip[15] = '\0';
    *sender_port = port;
    strncpy(message, msg.c_str(), message_size - 1);
    message[message_size - 1] = '\0';
    
    return 1;
}

}
