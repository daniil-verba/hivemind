// include/hivemind/Hivemind.hpp
#pragma once
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include "../storage/SimpleRegistry.hpp"

// Forward declarations
class Transport;
namespace hivemind { class BeaconClient; struct Packet; }

using MessageCallback = std::function<void(const std::string& sender, const std::string& message)>;

class Hivemind {
public:
    Hivemind();
    ~Hivemind();
    
    bool start(uint16_t port);
    void stop();
    
    // Отправка сообщений
    bool sendToIp(const std::string& ip, uint16_t port, const std::string& message);
    bool sendToUser(const std::string& username, const std::string& message);
    bool receive(std::string& ip, uint16_t& port, std::string& message);
    
    // Регистрация
    bool registerName(const std::string& name);
    bool findUser(const std::string& name, SimpleUser& user);
    
    // IP
    std::string getMyPublicIP() const;
    std::string getMyLocalIP() const;
    
    // Синхронизация
    void syncWithNetwork();
    void startAutoSync(int intervalSeconds);
    void stopAutoSync();
    
    // Настройки
    void enableHolePunching(bool enable);
    void setBeaconAddress(const std::string& ip, uint16_t port);
    void setMessageCallback(MessageCallback cb);
    
    // === НОВЫЕ МЕТОДЫ ===
    std::string getMyNodeId() const;
    bool requestUsersPack();           // Запросить users.pack с маяка
    bool isInUsersPack() const;        // Есть ли я в конфиге маяка?
    std::vector<SimpleUser> getNetworkUsers() const;
    bool sendViaRelay(const std::string& toUsername, const std::string& message);
    bool isBeaconConfigured() const;
    
    // Устаревшие методы API
    bool registerUsername(const std::string& username);
    bool resolveUsername(const std::string& username, SimpleUser& user);
    bool isBeaconConnected() const;
    bool requestBootstrap();
    
private:
    void generateNodeId();
    void recvLoop();
    void syncLoop();
    void onHolePunchDetected(const std::string& ip, uint16_t port);
    void processIncomingPacket(const hivemind::Packet& pkt,
                                const std::string& fromIp, uint16_t fromPort);
    
    std::unique_ptr<Transport> transport;
    std::unique_ptr<SimpleRegistry> registry;
    std::unique_ptr<hivemind::BeaconClient> beaconClient;
    
    uint8_t m_myNodeId[20];
    std::string m_myNodeIdStr;
    std::string m_myPublicIP;
    std::string m_myLocalIP;
    uint16_t m_myPort;
    std::string m_myUsername;
    
    std::atomic<bool> m_syncRunning;
    std::atomic<bool> m_recvRunning;
    std::thread m_syncThread;
    std::thread m_recvThread;
    int m_syncInterval;
    bool m_holePunchingEnabled;
    
    MessageCallback m_messageCallback;
    
    mutable std::mutex m_networkUsersMutex;
    std::vector<SimpleUser> m_networkUsers;  // Пользователи из users.pack
    bool m_usersPackReceived;
};