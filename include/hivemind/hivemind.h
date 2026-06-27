// include/hivemind/hivemind.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hivemind Hivemind;

// Создание/удаление
Hivemind* hivemind_create();
void hivemind_destroy(Hivemind* h);

// Запуск/остановка
int hivemind_start(Hivemind* h, uint16_t port);
void hivemind_stop(Hivemind* h);

// Отправка
int hivemind_send_to_ip(Hivemind* h, const char* ip, uint16_t port, const char* message);
int hivemind_send_to_user(Hivemind* h, const char* name, const char* message);
int hivemind_receive(Hivemind* h, char* sender_ip, uint16_t* sender_port,
                     char* message, size_t message_size);

// Регистрация
int hivemind_register_name(Hivemind* h, const char* name);
int hivemind_find_user(Hivemind* h, const char* name, char* ip, uint16_t* port);

// IP
int hivemind_get_public_ip(Hivemind* h, char* buffer, size_t buffer_size);
int hivemind_get_local_ip(Hivemind* h, char* buffer, size_t buffer_size);

// Синхронизация
void hivemind_sync_with_network(Hivemind* h);
void hivemind_start_auto_sync(Hivemind* h, int intervalSeconds);
void hivemind_stop_auto_sync(Hivemind* h);

// Настройки
void hivemind_enable_hole_punching(Hivemind* h, int enable);
void hivemind_set_beacon(Hivemind* h, const char* ip, uint16_t port);

// === НОВЫЕ ФУНКЦИИ ===
int hivemind_request_users_pack(Hivemind* h);
int hivemind_is_in_users_pack(Hivemind* h);
int hivemind_get_network_users_count(Hivemind* h);
int hivemind_get_network_user(Hivemind* h, int index, char* name, char* node_id, char* ip, uint16_t* port);
int hivemind_send_via_relay(Hivemind* h, const char* to_username, const char* message);
int hivemind_is_beacon_configured(Hivemind* h);
const char* hivemind_get_node_id(Hivemind* h);

typedef void (*HivemindMessageCallback)(const char* sender, const char* message);
void hivemind_set_message_callback(Hivemind* h, HivemindMessageCallback callback);

//OLD FUNCTIONS
int hivemind_get_node_id_old(Hivemind* h, char* buffer, size_t buffer_size);
void hivemind_add_anchor(Hivemind* h, const char* domain, uint16_t port);
void hivemind_enable_upnp(Hivemind* h, int enable);
int hivemind_is_upnp_available(Hivemind* h);

#ifdef __cplusplus
}
#endif