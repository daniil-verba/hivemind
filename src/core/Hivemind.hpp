// src/core/Hivemind.hpp
#pragma once

#include "../network/Transport.hpp"
#include "../network/Stun.hpp"
#include "../storage/UserRegistry.hpp"
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>

class Hivemind {
public:
    Hivemind();
    ~Hivemind();
    
    bool start(uint16_t port);
    void stop();
    
    bool sendToIp(const std::string& ip, uint16_t port, const std::string& message);
    bool receive(std::string& sender_ip, uint16_t& sender_port, std::string& message);
    
    bool registerName(const std::string& name);
    bool findUser(const std::string& name, UserEntry& entry);
    bool sendToUser(const std::string& name, const std::string& message);
    std::string getMyPublicIP();
    
    void syncWithNetwork();
    void startAutoSync(int intervalSeconds = 600);
    void stopAutoSync();
    
    void enableHolePunching(bool enable);
    void setHolePunchCallback(std::function<void(const std::string&, uint16_t)> cb);
    bool isHolePunchingEnabled() const { return m_holePunchingEnabled; }
    
    // ===== ЗАГЛУШКИ (для совместимости, но не используются) =====
    void addAnchor(const std::string&, uint16_t) {}  // Заглушка
    void setHTTPServer(const std::string&) {}        // Заглушка
    void enableUPnP(bool) {}                         // Заглушка
    bool isUPnPAvailable() const { return false; }
    bool addPortMapping(uint16_t, uint16_t = 0) { return false; }
    bool removePortMapping(uint16_t) { return false; }
    
private:
    void autoUpdateLoop();
    void onHolePunchDetected(const std::string& ip, uint16_t port);
    
    std::unique_ptr<Transport> transport;
    std::unique_ptr<UserRegistry> registry;
    
    bool m_holePunchingEnabled = true;
    
    std::string m_myName;
    std::string m_myPublicIP;
    uint16_t m_myPort = 0;
    
    std::atomic<bool> m_autoSyncRunning{false};
    std::thread m_autoSyncThread;
    
    std::function<void(const std::string&, uint16_t)> m_holePunchCallback;
};