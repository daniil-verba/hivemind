# 🚀 Установка и подключение

## Быстрый старт для разработчиков


### Шаг 1: Получите файлы библиотеки

Вам нужно получить два файла от разработчика:

[libhivemind.so](https://libhivemind.so/) (динамическая библиотека)  
hivemind.h (заголовочный файл)
### Шаг 2: Создайте структуру проекта

```bash
mkdir my_p2p_app
cd my_p2p_app
mkdir libs
cp /path/to/libhivemind.so libs/
cp /path/to/hivemind.h libs/
```

**Готовая структура:**

```text
my_p2p_app/
├── libs/
│   ├── libhivemind.so
│   └── hivemind.h
└── main.cpp
```

### Шаг 3: Напишите минимальное приложение

Создайте `main.cpp`:

```cpp

#include <iostream>
#include "libs/hivemind.h"
int main() {
    std::cout << "=== Hivemind Test ===" << std::endl;
    
    // 1. Создаём узел
    Hivemind* node = hivemind_create();
    if (!node) {
        std::cerr << "Failed to create node" << std::endl;
        return 1;
    }
    
    // 2. Запускаем на порту 9999
    if (!hivemind_start(node, 9999)) {
        std::cerr << "Failed to start on port 9999" << std::endl;
        hivemind_destroy(node);
        return 1;
    }
    
    std::cout << "✓ Hivemind started on port 9999" << std::endl;
    
    // 3. Отправляем тестовое сообщение
    hivemind_send_to_ip(node, "127.0.0.1", 9999, "Hello P2P!");
    std::cout << "✓ Test message sent" << std::endl;
    
    // 4. Пытаемся получить сообщение
    char sender_ip[16];
    uint16_t sender_port;
    char message[4096];
    
    if (hivemind_receive(node, sender_ip, &sender_port, message, sizeof(message))) {
        std::cout << "✓ Received: " << message << std::endl;
    }
    
    // 5. Останавливаем и чистим
    hivemind_stop(node);
    hivemind_destroy(node);
    
    return 0;
}
```

### Шаг 4: Соберите приложение

#### Вариант A: Прямая компиляция (простой)

```bash
g++ main.cpp -I libs -L libs -l hivemind -pthread -o app
```

#### Вариант B: Использование CMake (рекомендуется)

Создайте `CMakeLists.txt`:

```cmake

cmake_minimum_required(VERSION 3.10)
project(MyApp)
set(CMAKE_CXX_STANDARD 17)
add_executable(app main.cpp)
target_include_directories(app PRIVATE libs)
target_link_directories(app PRIVATE libs)
target_link_libraries(app hivemind pthread)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libhivemind.so
        ${CMAKE_CURRENT_BINARY_DIR}
)
```

Сборка:

```bash
mkdir build && cd build
cmake ..
make
```
### Шаг 5: Запустите приложение

```bash # Вариант 1: Указать путь к библиотеке
LD_LIBRARY_PATH=libs ./app
# Вариант 2: Если собирали через CMake
cd build && ./app
# Вариант 3: Установить в систему (требует прав)
sudo cp libs/libhivemind.so /usr/local/lib/
sudo ldconfig
./app
```

## 🐛 Частые проблемы

### Ошибка: `libhivemind.so: cannot open shared object file`

**Решение:**

```bash

# Временное решение
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libs
./app
# Или скопировать рядом с бинарником
cp libs/libhivemind.so .
./app
```
### Ошибка: `Address already in use`

**Решение:**

```bash

# Порт занят, используйте другой порт
# Или убейте процесс, использующий порт
sudo lsof -i :9999
sudo kill -9 <PID>
```
### Ошибка компиляции: `hivemind.h: No such file`

**Решение:**

```bash

# Проверьте путь в include
g++ main.cpp -I libs -L libs -l hivemind -pthread -o app
#              ^^^^^^ путь к папке, где лежит hivemind.h
```

## ✅ Что дальше?

- Изучите [API Reference](api/hivemind.md)
    
- Посмотрите [примеры приложений](examples/platform_messenger.md)
    
- Напишите свой P2P чат!
    

## 📞 Поддержка

По вопросам лицензирования и коммерческого использования:  
**Email:** daniilverba123@gmail.com