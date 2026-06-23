/*
 * Hivemind — P2P библиотека для децентрализованных приложений
 * 
 * Пример использования:
 * 
 *     Hivemind hivemind;
 *     hivemind.start(9999);
 *     hivemind.registerName("Alice");
 *     hivemind.sendMessage("Bob", "Hello!");
 * 
 */

 // include/hivemind/hivemind.h
#ifndef HIVEMIND_H
#define HIVEMIND_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle
typedef struct Hivemind Hivemind;

// Создание и уничтожение
Hivemind* hivemind_create(void);
void hivemind_destroy(Hivemind* h);

// Запуск/остановка
int hivemind_start(Hivemind* h, uint16_t port);
void hivemind_stop(Hivemind* h);

// Отправка сообщения по IP (прямая, без защиты)
int hivemind_send_to_ip(Hivemind* h, const char* ip, uint16_t port, const char* message);

// Получение сообщения (неблокирующий)
// Возвращает 1 если сообщение получено, иначе 0
int hivemind_receive(Hivemind* h, char* sender_ip, uint16_t* sender_port, 
                     char* message, size_t message_size);

#ifdef __cplusplus
}
#endif

#endif