# 🧠 Hivemind — P2P библиотека для обмена сообщениями
**Простая, быстрая, децентрализованная.**
## 📦 Что вы получите
- `libhivemind.so` — динамическая библиотека
- `hivemind.h` — заголовочный файл
## 🚀 Быстрый старт (3 минуты)
### 1. Скопируйте файлы в ваш проект

```text
your_project/  
├── libs/  
│ ├── [libhivemind.so](https://libhivemind.so/)  
│ └── hivemind.h  
└── main.cpp
```

### 2. Напишите простой мессенджер
```cpp
#include <iostream>
#include <thread>
#include "libs/hivemind.h"
int main() {
    Hivemind* node = hivemind_create();
    hivemind_start(node, 9999);
    
    std::cout << "P2P node started on port 9999" << std::endl;
    std::cout << "Send messages with: /send IP PORT MESSAGE" << std::endl;
    
    // ... ваш код
    
    hivemind_stop(node);
    hivemind_destroy(node);
}
```
### 3. Соберите и запустите

```bash

# Сборка
g++ main.cpp -I libs -L libs -l hivemind -pthread -o app
# Запуск
LD_LIBRARY_PATH=libs ./app
```

## 📚 Документация

- [Установка и подключение](https://getting_started.md/)
    
- [API Reference](https://api/hivemind.md)
    
- [Примеры приложений](https://examples/platform_messenger.md)
    

## 💡 Для чего нужна Hivemind?

- ✅ P2P мессенджеры
    
- ✅ Чат в играх
    
- ✅ Обмен данными между устройствами
    
- ✅ Децентрализованные приложения
    

## 📄 Лицензия

- Базовая версия: MIT (бесплатно)
    
- Расширения: коммерческая лицензия
    

**Разработчик:** Daniil Verba | GitHub: @daniil-verba  