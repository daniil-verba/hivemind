# 🧠 Hivemind

**Децентрализованная P2P-библиотека для построения распределённых приложений.**

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20Android-lightgrey)]()

---

## 🔥 Что это?

**Hivemind** — это библиотека, которая даёт любому приложению возможность общаться напрямую (P2P) без центральных серверов.

- Мессенджер → использует Hivemind для обмена сообщениями.
- Онлайн-игра → использует Hivemind для внутриигрового чата.
- Сайт поддержки → использует Hivemind для чата с клиентами.
- Torrent-клиент → использует Hivemind для поиска пиров.

**Все приложения используют одну сеть. Каждое новое приложение усиливает все остальные.**

---

## 🎯 Концепция

```
┌─────────────────────────────────────────────────────────────────┐
│                        HIVEMIND                                │
│              (децентрализованная P2P-сеть)                     │
│   Каждый узел = хранилище IP + маршрутизатор + ретранслятор    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ Используют
                              │
    ┌─────────────────────────┼─────────────────────────┐
    │                         │                         │
    ▼                         ▼                         ▼
┌─────────┐              ┌─────────┐              ┌─────────┐
│Platform │              │RPG Game │              │Website  │
│(чат)    │              │(игра)   │              │(поддержка)│
└─────────┘              └─────────┘              └─────────┘
```

---

## 🚀 Возможности

| Функция | Статус |
|---------|--------|
| 🌐 Отправка сообщений по IP | ✅ Готово |
| 🔒 Сквозное шифрование (X25519 + ChaCha20) | ⏳ В разработке |
| 🌐 P2P без серверов | ⏳ В разработке |
| 🧠 Kademlia DHT (миллионы узлов) | ⏳ В разработке |
| 🔄 Маршрутизация через ближайшие узлы | ⏳ В разработке |
| 💾 Хранение пар имя→IP в DHT | ⏳ В разработке |

---

## 📦 Установка

### Требования
- CMake 3.15+
- Компилятор с поддержкой C++20 (GCC 10+, Clang 12+, MSVC 2022)
- OpenSSL (для криптографии)

### Сборка
```bash
git clone https://github.com/daniil-verba/hivemind.git
cd hivemind
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Использование в проекте (CMake)
```cmake
find_package(hivemind REQUIRED)
target_link_libraries(your_app PRIVATE hivemind::hivemind)
```

---

## 📝 Пример использования

```cpp
#include <hivemind/hivemind.h>

int main() {
    // 1. Создаём узел
    Hivemind* h = hivemind_create();
    hivemind_start(h, 9999);
    
    // 2. Регистрируем имя
    hivemind_register_name(h, "Alice");
    
    // 3. Устанавливаем обработчик сообщений
    hivemind_set_message_callback(h, [](const char* from, const char* msg) {
        printf("[%s]: %s\n", from, msg);
    });
    
    // 4. Отправляем сообщение
    hivemind_send_message(h, "Bob", "Hello from Hivemind!");
    
    // 5. Ждём...
    sleep(3600);
    
    // 6. Завершаем
    hivemind_destroy(h);
    return 0;
}
```

---

## 📚 Документация

- [API Reference](docs/api/hivemind.md)
- [Getting Started](docs/getting_started.md)
- [Protocol Specification](docs/protocol/commands.md)
- [Architecture Overview](docs/architecture/overview.md)

---

## 🤝 Как помочь

| Область | Что нужно |
|---------|-----------|
| **Код** | C++ разработчики (сеть, криптография) |
| **Тестирование** | Запустить, найти баги |
| **Документация** | Дописать README, документацию, туториалы |
| **Портирование** | Windows, Android |

---

## 📄 Лицензия

Hivemind распространяется под **AGPL v3**.

Это означает:
- ✅ Вы можете использовать Hivemind бесплатно.
- ✅ Вы можете модифицировать и распространять код.
- ⚠️ Если вы используете Hivemind в **сетевом сервисе** (запускаете узел и позволяете другим подключаться), вы обязаны **открыть исходный код ваших изменений**.
- 💰 Для закрытых коммерческих проектов доступна **коммерческая лицензия** — пишите на `daniilverba123@gmail.com`.

## 📞 Контакты

- **Разработчик**: Daniil Verba
- **GitHub**: [daniil-verba](https://github.com/daniil-verba)
- **Почта**: daniilverba123@gmail.com

---

⭐ **Если проект полезен — поставьте звезду!**
