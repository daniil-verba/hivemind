# 📖 API Reference
## Полный список функций

### `Hivemind* hivemind_create(void)`

Создаёт новый экземпляр P2P узла.

**Возвращает:** Указатель на узел или `NULL` при ошибке.

**Пример:**
```cpp
Hivemind* node = hivemind_create();
if (!node) {
    std::cerr << "Failed to create node" << std::endl;
}
```
---

### `void hivemind_destroy(Hivemind* h)`

Уничтожает узел и освобождает все ресурсы.

**Параметры:**

- `h` — указатель на узел (полученный от `hivemind_create`)
    

**Пример:**

```cpp

Hivemind* node = hivemind_create();
// ... используем node ...
hivemind_destroy(node);
```
---

### `int hivemind_start(Hivemind* h, uint16_t port)`

Запускает сетевой узел на указанном UDP порту.

**Параметры:**

- `h` — указатель на узел
    
- `port` — номер порта (1-65535)
    

**Возвращает:**

- `1` — успех
    
- `0` — ошибка (порт занят, нет прав и т.д.)
    

**Пример:**

```cpp
if (!hivemind_start(node, 9999)) {
    std::cerr << "Can't start on port 9999" << std::endl;
}
```
---

### `void hivemind_stop(Hivemind* h)`

Останавливает узел. После остановки можно снова запустить `hivemind_start`.

**Параметры:**

- `h` — указатель на узел
    

**Пример:**
```cpp
hivemind_stop(node);
// ... можно запустить снова ...
hivemind_start(node, 8888);
```
---

### `int hivemind_send_to_ip(Hivemind* h, const char* ip, uint16_t port, const char* message)`

Отправляет текстовое сообщение напрямую по IP адресу.

**Параметры:**

- `h` — указатель на узел
    
- `ip` — IP адрес получателя (IPv4)
    
- `port` — порт получателя
    
- `message` — текст сообщения (UTF-8)
    

**Возвращает:**

- `1` — сообщение отправлено
    
- `0` — ошибка отправки
    

**Пример:**

```cpp
// Отправить сообщение на локальный узел
hivemind_send_to_ip(node, "127.0.0.1", 9999, "Hello!");
// Отправить в локальной сети
hivemind_send_to_ip(node, "192.168.1.100", 8888, "Привет!");
// Отправить в интернет (нужен публичный IP)
hivemind_send_to_ip(node, "123.45.67.89", 5555, "Hi from P2P!");
```
---

### `int hivemind_receive(Hivemind* h, char* sender_ip, uint16_t* sender_port, char* message, size_t message_size)`

Получает входящее сообщение. **Неблокирующая** функция.

**Параметры:**

- `h` — указатель на узел
    
- `sender_ip` — буфер для IP отправителя (минимум 16 байт)
    
- `sender_port` — указатель на переменную для порта отправителя
    
- `message` — буфер для текста сообщения
    
- `message_size` — размер буфера `message`
    

**Возвращает:**

- `1` — сообщение получено (данные записаны в буферы)
    
- `0` — нет новых сообщений
    

**Пример:**

```cpp
char ip[16];
uint16_t port;
char msg[4096];
if (hivemind_receive(node, ip, &port, msg, sizeof(msg))) {
    std::cout << "Получено от " << ip << ":" << port << " → " << msg << std::endl;
} else {
    std::cout << "Нет новых сообщений" << std::endl;
}
```
---

## 📝 Полный рабочий пример

```cpp

#include <iostream>
#include <thread>
#include <chrono>
#include "libs/hivemind.h"
int main() {
    // Создаём узел
    Hivemind* node = hivemind_create();
    
    // Запускаем на порту 9999
    if (!hivemind_start(node, 9999)) {
        std::cerr << "Failed to start" << std::endl;
        return 1;
    }
    
    std::cout << "Node running on port 9999" << std::endl;
    
    // Отправляем сообщение самому себе (тест)
    hivemind_send_to_ip(node, "127.0.0.1", 9999, "Test message");
    
    // Ждём немного
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Принимаем сообщение
    char ip[16];
    uint16_t port;
    char msg[4096];
    
    if (hivemind_receive(node, ip, &port, msg, sizeof(msg))) {
        std::cout << "Received: " << msg << " from " << ip << ":" << port << std::endl;
    }
    
    // Очистка
    hivemind_stop(node);
    hivemind_destroy(node);
    
    return 0;
}

## 🔄 Типичный паттерн использования

cpp

// 1. Инициализация
Hivemind* node = hivemind_create();
hivemind_start(node, PORT);
// 2. Главный цикл (отправка и приём)
while (running) {
    // Отправка
    if (need_to_send) {
        hivemind_send_to_ip(node, target_ip, target_port, message);
    }
    
    // Приём
    char ip[16];
    uint16_t port;
    char msg[4096];
    while (hivemind_receive(node, ip, &port, msg, sizeof(msg))) {
        process_incoming_message(ip, port, msg);
    }
    
    sleep(10);
}
// 3. Очистка
hivemind_stop(node);
hivemind_destroy(node);

## ⚠️ Важные замечания

1. **UDP протокол** — сообщения могут теряться. Для надёжной доставки нужны подтверждения (ACK).
    
2. **Размер сообщения** — ограничен ~64KB (UDP лимит).
    
3. **Кодировка** — ожидается UTF-8 текст.
    
4. **Неблокирующий receive** — всегда проверяйте возвращаемое значение.
    
5. **Потокобезопасность** — функции можно вызывать из разных потоков.
    

## 🚀 Продвинутое использование

### Многопоточный приём сообщений

cpp

std::atomic<bool> running(true);
Hivemind* node = hivemind_create();
void receiver_thread() {
    char ip[16];
    uint16_t port;
    char msg[4096];
    
    while (running) {
        if (hivemind_receive(node, ip, &port, msg, sizeof(msg))) {
            std::cout << "\n[MSG] " << ip << ":" << port << " → " << msg << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
int main() {
    hivemind_start(node, 9999);
    
    std::thread receiver(receiver_thread);
    
    // Основная логика...
    
    running = false;
    receiver.join();
    hivemind_stop(node);
    hivemind_destroy(node);
}
```

## 📞 Контакты для коммерческого использования

По вопросам лицензирования и получения закрытых расширений:

**Email:** daniilverba123@gmail.com  
**GitHub:** [https://github.com/daniil-verba](https://github.com/daniil-verba)