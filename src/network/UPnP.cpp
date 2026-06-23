// src/network/UPnP.cpp
#include "UPnP.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <chrono>
#include <algorithm> 

// Константы
constexpr const char* SSDP_ADDR = "239.255.255.250";
constexpr uint16_t SSDP_PORT = 1900;
constexpr int RECV_TIMEOUT_SEC = 3;
constexpr int MAX_BUFFER = 8192;

// ============================================================================
// Конструктор / Деструктор
// ============================================================================

UPnP::UPnP() = default;

UPnP::~UPnP() {
    for (const auto& mapping : m_activeMappings) {
        removePortMapping(mapping.first, mapping.second);
    }
}

// ============================================================================
// Вспомогательные функции
// ============================================================================

std::string UPnP::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool UPnP::parseURL(const std::string& url, std::string& host, uint16_t& port, std::string& path) {
    port = 80;
    path = "/";
    host = "";
    
    std::string copy = url;
    if (copy.find("http://") == 0) {
        copy = copy.substr(7);
    } else if (copy.find("https://") == 0) {
        copy = copy.substr(8);
        port = 443;
    } else {
        return false;
    }
    
    size_t slashPos = copy.find('/');
    size_t colonPos = copy.find(':');
    
    if (colonPos != std::string::npos && (slashPos == std::string::npos || colonPos < slashPos)) {
        host = copy.substr(0, colonPos);
        size_t portEnd = (slashPos != std::string::npos) ? slashPos : copy.size();
        std::string portStr = copy.substr(colonPos + 1, portEnd - colonPos - 1);
        port = static_cast<uint16_t>(std::stoi(portStr));
        if (slashPos != std::string::npos) {
            path = copy.substr(slashPos);
        } else {
            path = "/";
        }
    } else if (slashPos != std::string::npos) {
        host = copy.substr(0, slashPos);
        path = copy.substr(slashPos);
    } else {
        host = copy;
        path = "/";
    }
    
    return !host.empty();
}

bool UPnP::resolveHost(const std::string& host, std::string& ip) {
    struct hostent* he = gethostbyname(host.c_str());
    if (!he) return false;
    
    struct in_addr** addrList = (struct in_addr**)he->h_addr_list;
    if (!addrList || !addrList[0]) return false;
    
    ip = inet_ntoa(*addrList[0]);
    return true;
}

// ============================================================================
// HTTP Helper (без сложных парсеров)
// ============================================================================

std::string UPnP::httpGet(const std::string& url) {
    std::string host, path;
    uint16_t port;
    
    if (!parseURL(url, host, port, path)) return "";
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    std::string ip;
    if (!resolveHost(host, ip)) {
        close(sock);
        return "";
    }
    
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return "";
    }
    
    std::string request = 
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    send(sock, request.c_str(), request.size(), 0);
    
    std::string response;
    char buffer[MAX_BUFFER];
    while (true) {
        int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) break;
        buffer[received] = '\0';
        response += buffer;
    }
    
    close(sock);
    
    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    
    return response.substr(pos + 4);
}

std::string UPnP::httpPost(const std::string& url, const std::string& body) {
    std::string host, path;
    uint16_t port;
    
    if (!parseURL(url, host, port, path)) return "";
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";
    
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    std::string ip;
    if (!resolveHost(host, ip)) {
        close(sock);
        return "";
    }
    
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return "";
    }
    
    std::string request = 
        "POST " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Content-Type: text/xml; charset=\"utf-8\"\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "SOAPAction: \"" + m_serviceType + "\"\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    
    send(sock, request.c_str(), request.size(), 0);
    
    std::string response;
    char buffer[MAX_BUFFER];
    while (true) {
        int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) break;
        buffer[received] = '\0';
        response += buffer;
    }
    
    close(sock);
    
    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    
    return response.substr(pos + 4);
}

// ============================================================================
// XML парсер (без std::regex, простой ручной)
// ============================================================================

std::string UPnP::extractTagContent(const std::string& xml, const std::string& tag) {
    std::string openTag = "<" + tag + ">";
    std::string closeTag = "</" + tag + ">";
    
    size_t start = xml.find(openTag);
    if (start == std::string::npos) return "";
    
    size_t end = xml.find(closeTag, start);
    if (end == std::string::npos) return "";
    
    start += openTag.size();
    return trim(xml.substr(start, end - start));
}

bool UPnP::parseServiceDescription(const std::string& xml, std::string& controlURL, std::string& serviceType) {
    // Ищем любой из поддерживаемых типов служб
    std::string types[] = {
        "urn:schemas-upnp-org:service:WANIPConnection:1",
        "urn:schemas-upnp-org:service:WANIPConnection:2",
        "urn:schemas-upnp-org:service:WANPPPConnection:1"
    };
    
    for (const std::string& type : types) {
        if (xml.find(type) != std::string::npos) {
            // Ищем блок service для этого типа
            std::string searchTag = "<serviceType>" + type + "</serviceType>";
            size_t serviceStart = xml.find(searchTag);
            if (serviceStart == std::string::npos) continue;
            
            // Ищем начало блока service (ищем <service> перед этим)
            size_t blockStart = xml.rfind("<service>", serviceStart);
            if (blockStart == std::string::npos) continue;
            
            // Ищем конец блока service
            size_t blockEnd = xml.find("</service>", serviceStart);
            if (blockEnd == std::string::npos) continue;
            
            std::string serviceBlock = xml.substr(blockStart, blockEnd - blockStart + 10);
            
            // Извлекаем controlURL
            std::string url = extractTagContent(serviceBlock, "controlURL");
            if (!url.empty()) {
                controlURL = url;
                serviceType = type;
                return true;
            }
        }
    }
    
    return false;
}

// ============================================================================
// SSDP Discovery
// ============================================================================

bool UPnP::sendSSDPDiscovery() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;
    
    struct timeval tv;
    tv.tv_sec = RECV_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    
    std::string request = 
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: " + std::string(SSDP_ADDR) + ":" + std::to_string(SSDP_PORT) + "\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 3\r\n"
        "ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
        "\r\n";
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SSDP_PORT);
    inet_pton(AF_INET, SSDP_ADDR, &addr.sin_addr);
    
    sendto(sock, request.c_str(), request.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
    
    char buffer[MAX_BUFFER];
    struct sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    
    auto startTime = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(RECV_TIMEOUT_SEC);
    
    while (std::chrono::steady_clock::now() - startTime < timeout) {
        int received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, 
                               (struct sockaddr*)&fromAddr, &fromLen);
        
        if (received > 0) {
            buffer[received] = '\0';
            std::string response(buffer);
            
            if (response.find("200 OK") != std::string::npos) {
                // Извлекаем LOCATION
                size_t pos = response.find("LOCATION:");
                if (pos != std::string::npos) {
                    size_t end = response.find("\r\n", pos);
                    if (end != std::string::npos) {
                        std::string location = response.substr(pos + 9, end - pos - 9);
                        location = trim(location);
                        
                        // Сохраняем IP роутера
                        char routerIP[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &fromAddr.sin_addr, routerIP, INET_ADDRSTRLEN);
                        m_routerIP = routerIP;
                        
                        // Получаем описание сервиса
                        std::string xml = httpGet(location);
                        if (!xml.empty()) {
                            std::string controlURL, serviceType;
                            if (parseServiceDescription(xml, controlURL, serviceType)) {
                                // Формируем полный URL для контроля
                                if (controlURL.find("http://") != 0) {
                                    size_t lastSlash = location.find_last_of('/');
                                    if (lastSlash != std::string::npos) {
                                        std::string base = location.substr(0, lastSlash);
                                        if (controlURL[0] == '/') {
                                            m_controlURL = base + controlURL;
                                        } else {
                                            m_controlURL = base + "/" + controlURL;
                                        }
                                    }
                                } else {
                                    m_controlURL = controlURL;
                                }
                                
                                m_serviceType = serviceType;
                                m_discovered = true;
                                
                                close(sock);
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    close(sock);
    return false;
}

// ============================================================================
// Public API
// ============================================================================

bool UPnP::discover() {
    if (m_discovered) return true;
    
    std::cout << "[UPnP] Discovering IGD router..." << std::endl;
    
    if (sendSSDPDiscovery()) {
        std::cout << "[UPnP] Router found at: " << m_routerIP << std::endl;
        std::cout << "[UPnP] Control URL: " << m_controlURL << std::endl;
        
        // Пробуем получить внешний IP
        m_externalIP = getExternalIP();
        if (!m_externalIP.empty()) {
            std::cout << "[UPnP] External IP: " << m_externalIP << std::endl;
        }
        return true;
    }
    
    std::cout << "[UPnP] No IGD router found" << std::endl;
    return false;
}

std::string UPnP::getExternalIP() {
    if (!m_discovered || m_controlURL.empty()) return "";
    
    std::string soapBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:GetExternalIPAddress xmlns:u=\"" + m_serviceType + "\">\r\n"
        "    </u:GetExternalIPAddress>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    
    std::string response = httpPost(m_controlURL, soapBody);
    if (response.empty()) return "";
    
    std::string ip = extractTagContent(response, "NewExternalIPAddress");
    return trim(ip);
}

bool UPnP::addPortMapping(uint16_t internalPort, uint16_t externalPort,
                          const std::string& protocol, const std::string& description) {
    if (!m_discovered) {
        if (!discover()) return false;
    }
    
    if (externalPort == 0) externalPort = internalPort;
    
    // Получаем локальный IP
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return false;
    }
    
    struct sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    if (getsockname(sock, (struct sockaddr*)&localAddr, &addrLen) < 0) {
        close(sock);
        return false;
    }
    close(sock);
    
    char localIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &localAddr.sin_addr, localIP, INET_ADDRSTRLEN);
    
    std::cout << "[UPnP] Adding mapping: " << externalPort << " (" << protocol 
              << ") -> " << localIP << ":" << internalPort << std::endl;
    
    std::string soapBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:AddPortMapping xmlns:u=\"" + m_serviceType + "\">\r\n"
        "      <NewRemoteHost></NewRemoteHost>\r\n"
        "      <NewExternalPort>" + std::to_string(externalPort) + "</NewExternalPort>\r\n"
        "      <NewProtocol>" + protocol + "</NewProtocol>\r\n"
        "      <NewInternalPort>" + std::to_string(internalPort) + "</NewInternalPort>\r\n"
        "      <NewInternalClient>" + localIP + "</NewInternalClient>\r\n"
        "      <NewEnabled>1</NewEnabled>\r\n"
        "      <NewPortMappingDescription>" + description + "</NewPortMappingDescription>\r\n"
        "      <NewLeaseDuration>0</NewLeaseDuration>\r\n"
        "    </u:AddPortMapping>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    
    std::string response = httpPost(m_controlURL, soapBody);
    
    // Проверяем успех (ищем ошибку)
    bool success = response.find("error") == std::string::npos && !response.empty();
    
    if (success) {
        m_activeMappings.push_back({externalPort, protocol});
        std::cout << "[UPnP] ✓ Port mapping added" << std::endl;
    } else {
        std::cout << "[UPnP] ✗ Failed to add port mapping" << std::endl;
    }
    
    return success;
}

bool UPnP::removePortMapping(uint16_t externalPort, const std::string& protocol) {
    if (!m_discovered) return false;
    
    std::cout << "[UPnP] Removing mapping: " << externalPort << " (" << protocol << ")" << std::endl;
    
    std::string soapBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:DeletePortMapping xmlns:u=\"" + m_serviceType + "\">\r\n"
        "      <NewRemoteHost></NewRemoteHost>\r\n"
        "      <NewExternalPort>" + std::to_string(externalPort) + "</NewExternalPort>\r\n"
        "      <NewProtocol>" + protocol + "</NewProtocol>\r\n"
        "    </u:DeletePortMapping>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    
    std::string response = httpPost(m_controlURL, soapBody);
    
    bool success = response.find("error") == std::string::npos && !response.empty();
    
    if (success) {
        std::cout << "[UPnP] ✓ Mapping removed" << std::endl;
        // Удаляем из активных
        m_activeMappings.erase(
            std::remove_if(m_activeMappings.begin(), m_activeMappings.end(),
                [externalPort, &protocol](const auto& m) {
                    return m.first == externalPort && m.second == protocol;
                }),
            m_activeMappings.end()
        );
    } else {
        std::cout << "[UPnP] ✗ Failed to remove mapping" << std::endl;
    }
    
    return success;
}