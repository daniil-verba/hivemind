// include/hivemind/hivemind.h
#ifndef HIVEMIND_H
#define HIVEMIND_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hivemind Hivemind;

// ====== Базовый API ======
Hivemind* hivemind_create();
void hivemind_destroy(Hivemind* h);
int hivemind_start(Hivemind* h, uint16_t port);
void hivemind_stop(Hivemind* h);

int hivemind_send_to_ip(Hivemind* h, const char* ip, uint16_t port, const char* message);
int hivemind_receive(Hivemind* h, char* sender_ip, uint16_t* sender_port,
                     char* message, size_t message_size);

// ====== Zero-Config API ======
int hivemind_register_name(Hivemind* h, const char* name);
int hivemind_find_user(Hivemind* h, const char* name, char* ip, uint16_t* port);
int hivemind_send_to_user(Hivemind* h, const char* name, const char* message);
int hivemind_get_public_ip(Hivemind* h, char* buffer, size_t buffer_size);

// ====== Hole Punching ======
void hivemind_enable_hole_punching(Hivemind* h, int enable);

// ====== Синхронизация ======
void hivemind_sync_with_network(Hivemind* h);
void hivemind_start_auto_sync(Hivemind* h, int intervalSeconds);
void hivemind_stop_auto_sync(Hivemind* h);

// ====== ЗАГЛУШКИ (не используются, но оставлены для совместимости) ======
void hivemind_add_anchor(Hivemind* h, const char* domain, uint16_t port);
void hivemind_set_http_server(Hivemind* h, const char* url);
void hivemind_enable_upnp(Hivemind* h, int enable);
int hivemind_is_upnp_available(Hivemind* h);

#ifdef __cplusplus
}
#endif

#endif // HIVEMIND_H