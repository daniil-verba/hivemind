// src/hivemind.cpp
#include "hivemind/hivemind.h"
#include "core/Hivemind.hpp"
#include <cstring>
#include <string>
#include <vector>

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

int hivemind_register_name(Hivemind* h, const char* name) {
    return h->registerName(name) ? 1 : 0;
}

int hivemind_find_user(Hivemind* h, const char* name, char* ip, uint16_t* port) {
    SimpleUser user;
    if (!h->findUser(name, user)) return 0;
    strncpy(ip, user.ip.c_str(), 63);
    ip[63] = '\0';
    *port = user.port;
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

int hivemind_get_local_ip(Hivemind* h, char* buffer, size_t buffer_size) {
    if (!h || !buffer || buffer_size == 0) return 0;
    std::string ip = h->getMyLocalIP();
    if (ip.empty()) return 0;
    strncpy(buffer, ip.c_str(), buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return 1;
}

void hivemind_sync_with_network(Hivemind* h) {
    h->syncWithNetwork();
}

void hivemind_start_auto_sync(Hivemind* h, int intervalSeconds) {
    h->startAutoSync(intervalSeconds);
}

void hivemind_stop_auto_sync(Hivemind* h) {
    h->stopAutoSync();
}

void hivemind_enable_hole_punching(Hivemind* h, int enable) {
    h->enableHolePunching(enable != 0);
}

void hivemind_set_beacon(Hivemind* h, const char* ip, uint16_t port) {
    if (!h || !ip) return;
    h->setBeaconAddress(ip, port);
}

// === НОВЫЕ ФУНКЦИИ ===

int hivemind_request_users_pack(Hivemind* h) {
    if (!h) return 0;
    return h->requestUsersPack() ? 1 : 0;
}

int hivemind_is_in_users_pack(Hivemind* h) {
    if (!h) return 0;
    return h->isInUsersPack() ? 1 : 0;
}

int hivemind_get_network_users_count(Hivemind* h) {
    if (!h) return 0;
    return static_cast<int>(h->getNetworkUsers().size());
}

int hivemind_get_network_user(Hivemind* h, int index, char* name, char* node_id, char* ip, uint16_t* port) {
    if (!h || index < 0) return 0;
    auto users = h->getNetworkUsers();
    if (index >= static_cast<int>(users.size())) return 0;
    
    const auto& user = users[index];
    if (name) {
        strncpy(name, user.name.c_str(), 63);
        name[63] = '\0';
    }
    if (node_id) {
        strncpy(node_id, user.nodeId.c_str(), 41);
        node_id[40] = '\0';
    }
    if (ip) {
        strncpy(ip, user.ip.c_str(), 63);
        ip[63] = '\0';
    }
    if (port) {
        *port = user.port;
    }
    return 1;
}

int hivemind_send_via_relay(Hivemind* h, const char* to_username, const char* message) {
    if (!h || !to_username || !message) return 0;
    return h->sendViaRelay(to_username, message) ? 1 : 0;
}

int hivemind_is_beacon_configured(Hivemind* h) {
    if (!h) return 0;
    return h->isBeaconConfigured() ? 1 : 0;
}

const char* hivemind_get_node_id(Hivemind* h) {
    if (!h) return "";
    static std::string id;
    id = h->getMyNodeId();
    return id.c_str();
}

// === УСТАРЕВШИЕ ФУНКЦИИ ===

int hivemind_register_username(Hivemind* h, const char* username) {
    if (!h || !username) return 0;
    return h->registerUsername(username) ? 1 : 0;
}

int hivemind_resolve_username(Hivemind* h, const char* username, char* ip, uint16_t* port) {
    if (!h || !username || !ip || !port) return 0;
    SimpleUser user;
    if (!h->resolveUsername(username, user)) return 0;
    strncpy(ip, user.ip.c_str(), 63);
    ip[63] = '\0';
    *port = user.port;
    return 1;
}

int hivemind_is_beacon_connected(Hivemind* h) {
    if (!h) return 0;
    return h->isBeaconConnected() ? 1 : 0;
}

int hivemind_request_bootstrap(Hivemind* h) {
    if (!h) return 0;
    return h->requestBootstrap() ? 1 : 0;
}

int hivemind_get_node_id_old(Hivemind* h, char* buffer, size_t buffer_size) {
    if (!h || !buffer || buffer_size < 41) return 0;
    strncpy(buffer, "0000000000000000000000000000000000000000", 41);
    return 1;
}

void hivemind_add_anchor(Hivemind* h, const char* domain, uint16_t port) {
    (void)h; (void)domain; (void)port;
}

void hivemind_enable_upnp(Hivemind* h, int enable) {
    (void)h; (void)enable;
}

int hivemind_is_upnp_available(Hivemind* h) {
    (void)h;
    return 0;
}

} // extern "C"