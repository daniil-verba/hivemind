# 💬 Пример: Полноценный P2P мессенджер
## Полный код мессенджера

Создайте `messenger.cpp`:

```cpp
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include "libs/hivemind.h"
std::atomic<bool> running(true);
Hivemind* g_node = nullptr;
// Цвета для красивого вывода (опционально)
const std::string COLOR_RESET = "\033[0m";
const std::string COLOR_GREEN = "\033[32m";
const std::string COLOR_CYAN = "\033[36m";
const std::string COLOR_YELLOW = "\033[33m";
const std::string COLOR_RED = "\033[31m";
void print_banner() {
    std::cout << COLOR_CYAN << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║     HIVEMIND P2P MESSENGER v1.0          ║\n";
    std::cout << "║     Децентрализованный мессенджер        ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET << std::endl;
}
void print_help() {
    std::cout << COLOR_YELLOW << "\nCommands:\n";
    std::cout << COLOR_GREEN << "  /send <IP> <PORT> <msg>" << COLOR_RESET << "  - Send message\n";
    std::cout << COLOR_GREEN << "  /help" << COLOR_RESET << "                   - Show this help\n";
    std::cout << COLOR_GREEN << "  /quit" << COLOR_RESET << "                   - Exit messenger\n";
    std::cout << std::endl;
}
void receive_loop() {
    char sender_ip[16];
    uint16_t sender_port;
    char message[4096];
    
    while (running) {
        if (hivemind_receive(g_node, sender_ip, &sender_port, message, sizeof(message))) {
            std::cout << "\n" << COLOR_GREEN << "[✓] " << sender_ip << ":" << sender_port 
                      << COLOR_RESET << "\n  → " << message << std::endl;
            std::cout << COLOR_CYAN << "> " << COLOR_RESET << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
int main() {
    print_banner();
    
    // Создаём узел
    g_node = hivemind_create();
    if (!g_node) {
        std::cerr << COLOR_RED << "❌ Failed to create Hivemind node" << COLOR_RESET << std::endl;
        return 1;
    }
    
    // Ввод порта
    int port;
    std::cout << "Enter your port (e.g., 9999): ";
    std::cin >> port;
    
    // Запуск
    if (!hivemind_start(g_node, port)) {
        std::cerr << COLOR_RED << "❌ Failed to start on port " << port << COLOR_RESET << std::endl;
        hivemind_destroy(g_node);
        return 1;
    }
    
    std::cout << COLOR_GREEN << "✅ Hivemind started on port " << port << COLOR_RESET << std::endl;
    
    // Определяем локальный IP (простой способ)
    std::string local_ip = "127.0.0.1"; // В реальности можно получить через ifconfig
    
    std::cout << COLOR_CYAN << "ℹ️  Your address: " << local_ip << ":" << port << COLOR_RESET << std::endl;
    
    print_help();
    
    // Запускаем поток приёма
    std::thread receiver(receive_loop);
    std::cin.ignore();
    
    // Главный цикл
    std::string input;
    while (running) {
        std::cout << COLOR_CYAN << "> " << COLOR_RESET;
        std::getline(std::cin, input);
        
        if (input == "/quit") {
            running = false;
            break;
        }
        else if (input == "/help") {
            print_help();
        }
        else if (input.substr(0, 5) == "/send") {
            // Парсинг: /send 192.168.1.100 9999 Hello world
            std::string rest = input.substr(6);
            
            size_t first_space = rest.find(' ');
            if (first_space == std::string::npos) {
                std::cout << COLOR_RED << "❌ Usage: /send <IP> <PORT> <message>" << COLOR_RESET << std::endl;
                continue;
            }
            
            std::string target_ip = rest.substr(0, first_space);
            rest = rest.substr(first_space + 1);
            
            size_t second_space = rest.find(' ');
            if (second_space == std::string::npos) {
                std::cout << COLOR_RED << "❌ Usage: /send <IP> <PORT> <message>" << COLOR_RESET << std::endl;
                continue;
            }
            
            std::string port_str = rest.substr(0, second_space);
            std::string message = rest.substr(second_space + 1);
            
            try {
                uint16_t target_port = static_cast<uint16_t>(std::stoi(port_str));
                
                if (hivemind_send_to_ip(g_node, target_ip.c_str(), target_port, message.c_str())) {
                    std::cout << COLOR_GREEN << "✅ Sent to " << target_ip << ":" << target_port << COLOR_RESET << std::endl;
                } else {
                    std::cout << COLOR_RED << "❌ Failed to send" << COLOR_RESET << std::endl;
                }
            } catch (...) {
                std::cout << COLOR_RED << "❌ Invalid port number" << COLOR_RESET << std::endl;
            }
        }
        else if (!input.empty()) {
            std::cout << COLOR_RED << "❌ Unknown command. Type /help for commands" << COLOR_RESET << std::endl;
        }
    }
    
    // Очистка
    std::cout << COLOR_YELLOW << "Shutting down..." << COLOR_RESET << std::endl;
    hivemind_stop(g_node);
    hivemind_destroy(g_node);
    receiver.join();
    
    std::cout << COLOR_GREEN << "Goodbye!" << COLOR_RESET << std::endl;
    return 0;
}
```
## Сборка мессенджера

### Вариант 1: Простая компиляция

```bash
# Компилируем
g++ messenger.cpp -I libs -L libs -l hivemind -pthread -o messenger
# Запускаем
LD_LIBRARY_PATH=libs ./messenger
```
### Вариант 2: CMake (рекомендуется)

Создайте `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(Messenger)
set(CMAKE_CXX_STANDARD 17)
add_executable(messenger messenger.cpp)
target_include_directories(messenger PRIVATE libs)
target_link_directories(messenger PRIVATE libs)
target_link_libraries(messenger hivemind pthread)
add_custom_command(TARGET messenger POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libhivemind.so
        ${CMAKE_CURRENT_BINARY_DIR}
)
```

Сборка и запуск:

```bash

mkdir build && cd build
cmake ..
make
./messenger
```

## Использование мессенджера

### Запуск двух экземпляров

**Терминал 1 (Пользователь А):**

```bash
./messenger
# Enter port: 9999
```

**Терминал 2 (Пользователь Б):**

```bash
./messenger
# Enter port: 8888
```

### Отправка сообщений

**От пользователя Б к А:**

```text
/send 127.0.0.1 9999 Привет, Алиса!
```

**Ответ от А к Б:**

```text
/send 127.0.0.1 8888 Привет, Боб! Как дела?
```
### Пример сессии

```text

╔══════════════════════════════════════════╗
║     HIVEMIND P2P MESSENGER v1.0          ║
║     Децентрализованный мессенджер        ║
╚══════════════════════════════════════════╝
Enter your port (e.g., 9999): 9999
✅ Hivemind started on port 9999
ℹ️  Your address: 127.0.0.1:9999
Commands:
  /send <IP> <PORT> <msg>  - Send message
  /help                    - Show this help
  /quit                    - Exit messenger
> /send 127.0.0.1 8888 Привет!
[✓] 127.0.0.1:8888
  → Привет!
> 
```

## Тестирование в локальной сети

1. Запустите на двух компьютерах в одной сети
    
2. Узнайте IP адреса: `ip addr` или `ifconfig`
    
3. Отправляйте сообщения напрямую
    

**Компьютер 1 (IP: 192.168.1.100, порт: 9999)**  
**Компьютер 2 (IP: 192.168.1.101, порт: 8888)**

```bash
# На компьютере 2 отправляем на компьютер 1
/send 192.168.1.100 9999 Hello from the other side!
```

## Возможные улучшения

1. **История сообщений** - сохранять в файл
    
2. **Список контактов** - хранить часто используемые IP
    
3. **Автоопределение IP** - получить реальный локальный IP
    
4. **Шифрование** - добавить сквозное шифрование
    
5. **Файлы** - передача файлов
    
6. **Групповые чаты** - широковещательные сообщения
    

## Решение проблем

### Не вижу сообщения от другого пользователя

1. Проверьте, что оба узла запущены
    
2. Проверьте IP адрес (должен быть правильным)
    
3. Убедитесь, что порты не заблокированы брандмауэром
    
4. Попробуйте отправить себе сообщение (`/send 127.0.0.1 9999 test`)
    

### Ошибка "Address already in use"

Порт уже занят другой программой. Используйте другой порт:

```bash

# Найдите свободный порт
netstat -tuln | grep :9999
# Используйте другой порт, например 8888
```

### Ошибка при отправке

Убедитесь, что целевой узел запущен и слушает правильный порт.