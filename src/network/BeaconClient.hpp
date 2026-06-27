// include/hivemind/BeaconClient.hpp
#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include "PacketManager.hpp"

class Transport;

namespace hivemind {

using BeaconResponseCallback = std::function<void(bool success, const NodeInfo& info)>;
using BootstrapCallback = std::function<void(const std::vector<NodeInfo>& nodes)>;

class BeaconClient {
public:
    explicit BeaconClient(Transport* transport);
    
    void setBeaconAddress(const std::string& ip, uint16_t port);
    std::string getBeaconIp() const { return m_beaconIp; }
    uint16_t getBeaconPort() const { return m_beaconPort; }
    
    bool isConfigured() const { return !m_beaconIp.empty() && m_beaconPort != 0; }
    
    bool registerUsername(const std::string& username, const NodeInfo& myInfo, BeaconResponseCallback callback);
    bool resolveUsername(const std::string& username, BeaconResponseCallback callback);
    bool requestBootstrap(const uint8_t myNodeId[20], BootstrapCallback callback);
    bool pingBeacon();
    
    void handlePacket(const Packet& pkt, const std::string& fromIp, uint16_t fromPort);
    
private:
    Transport* m_transport;
    std::string m_beaconIp;
    uint16_t m_beaconPort;
    
    BeaconResponseCallback m_pendingUsernameCallback;
    BootstrapCallback m_pendingBootstrapCallback;
    
    std::atomic<bool> m_waitingResponse;
    std::chrono::steady_clock::time_point m_lastRequestTime;
};

} // namespace hivemind