// src/hivemind.cpp
#include "hivemind/hivemind.h"
#include "core/Hivemind.hpp"
#include <cstring>
#include <string>

extern "C" {

// ====== Базовый API ======
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

// ====== Zero-Config API ======
int hivemind_register_name(Hivemind* h, const char* name) {
    return h->registerName(name) ? 1 : 0;
}

int hivemind_find_user(Hivemind* h, const char* name, char* ip, uint16_t* port) {
    UserEntry entry;
    if (!h->findUser(name, entry)) return 0;
    
    strncpy(ip, entry.publicIP.c_str(), 63);
    ip[63] = '\0';
    *port = entry.port;
    return 1;
}

int hivemind_send_to_user(Hivemind* h, const char* name, const char* message) {
    return h->sendToUser(name, message) ? 1 : 0;
}

int hivemind_get_public_ip(Hivemind* h, char* buffer, size_t buffer_size) {
    if (!h || !buffer || buffer_size == 0) return 0;
    
    std::string ip = h->getMyPublicIP();
    if (ip.empty()) return 0;
    
    strncpy(buffer, ip.c_str(), buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return 1;
}

// ====== Hole Punching ======
void hivemind_enable_hole_punching(Hivemind* h, int enable) {
    h->enableHolePunching(enable != 0);
}

// ====== Синхронизация ======
void hivemind_sync_with_network(Hivemind* h) {
    h->syncWithNetwork();
}

void hivemind_start_auto_sync(Hivemind* h, int intervalSeconds) {
    h->startAutoSync(intervalSeconds);
}

void hivemind_stop_auto_sync(Hivemind* h) {
    h->stopAutoSync();
}

// ====== ЗАГЛУШКИ ======
void hivemind_add_anchor(Hivemind* h, const char* domain, uint16_t port) {
    // Заглушка — ничего не делает
    (void)h; (void)domain; (void)port;
}

void hivemind_set_http_server(Hivemind* h, const char* url) {
    // Заглушка — ничего не делает
    (void)h; (void)url;
}

void hivemind_enable_upnp(Hivemind* h, int enable) {
    // Заглушка — ничего не делает
    (void)h; (void)enable;
}

int hivemind_is_upnp_available(Hivemind* h) {
    (void)h;
    return 0;
}

} // extern "C"