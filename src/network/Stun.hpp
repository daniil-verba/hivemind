// src/network/Stun.hpp
#pragma once

#include <string>
#include <cstdint>
#include <vector>

class StunClient {
public:
    /**
     * Основной метод получения публичного IP
     * Приоритет:
     *   1. Российские STUN-сервера
     *   2. Google STUN-сервера (зарубежные)
     *   3. Локальный IP (для локальной сети)
     */
    static std::string getPublicIP();
    
    /**
     * Получить IP через STUN-сервер
     */
    static std::string getPublicIPViaStun(const std::string& server, uint16_t port);
    
    /**
     * Получить локальный IP (для локальной сети)
     */
    static std::string getLocalIP();
    
private:
    // Вспомогательные функции
    static bool resolveHost(const std::string& host, std::string& ip);
};