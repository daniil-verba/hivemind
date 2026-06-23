// src/network/UPnP.hpp
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>

class UPnP {
public:
    UPnP();
    ~UPnP();

    bool discover();
    std::string getExternalIP();
    bool addPortMapping(uint16_t internalPort, 
                        uint16_t externalPort = 0,
                        const std::string& protocol = "UDP",
                        const std::string& description = "Hivemind P2P");
    bool removePortMapping(uint16_t externalPort, const std::string& protocol = "UDP");
    bool isAvailable() const { return !m_controlURL.empty() && m_discovered; }

private:
    // SSDP discovery (UDP multicast)
    bool sendSSDPDiscovery();
    bool parseSSDPResponse(const std::string& response, std::string& locationURL);
    
    // HTTP helpers (simple, без regex)
    std::string httpGet(const std::string& url);
    std::string httpPost(const std::string& url, const std::string& body);
    
    // Простой XML парсер (без regex)
    std::string extractTagContent(const std::string& xml, const std::string& tag);
    bool parseServiceDescription(const std::string& xml, std::string& controlURL, std::string& serviceType);
    
    // Вспомогательные функции
    static bool resolveHost(const std::string& host, std::string& ip);
    static bool parseURL(const std::string& url, std::string& host, uint16_t& port, std::string& path);
    static std::string trim(const std::string& str);

    // Данные
    std::string m_controlURL;
    std::string m_serviceType;
    std::string m_routerIP;
    std::string m_externalIP;
    std::atomic<bool> m_discovered{false};
    std::vector<std::pair<uint16_t, std::string>> m_activeMappings;
};