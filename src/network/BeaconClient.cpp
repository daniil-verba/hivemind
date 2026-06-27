// src/network/BeaconClient.cpp
#include "BeaconClient.hpp"
#include "network/Transport.hpp"
#include <iostream>
#include <chrono>

namespace hivemind {

BeaconClient::BeaconClient(Transport* transport)
    : m_transport(transport), m_beaconPort(0), m_waitingResponse(false) {}

void BeaconClient::setBeaconAddress(const std::string& ip, uint16_t port) {
    m_beaconIp = ip;
    m_beaconPort = port;
    std::cout << "[BeaconClient] Beacon set to " << ip << ":" << port << std::endl;
}

bool BeaconClient::registerUsername(const std::string& username, const NodeInfo& myInfo, BeaconResponseCallback callback) {
    std::cout << "[BeaconClient] registerUsername() called for: " << username << std::endl;
    
    if (!isConfigured()) {
        std::cerr << "[BeaconClient] ERROR: not configured!" << std::endl;
        return false;
    }
    if (!m_transport) {
        std::cerr << "[BeaconClient] ERROR: transport is null!" << std::endl;
        return false;
    }
    
    std::cout << "[BeaconClient] Creating packet..." << std::endl;
    auto data = createRegisterUsernamePacket(username, myInfo);
    std::cout << "[BeaconClient] Packet size: " << data.size() << " bytes" << std::endl;
    
    m_pendingUsernameCallback = callback;
    m_waitingResponse = true;
    m_lastRequestTime = std::chrono::steady_clock::now();
    
    std::cout << "[BeaconClient] Sending to " << m_beaconIp << ":" << m_beaconPort << std::endl;
    bool sent = m_transport->sendTo(m_beaconIp, m_beaconPort, std::string(data.begin(), data.end()));
    
    if (sent) {
        std::cout << "[BeaconClient] ✓ Sent REGISTER_USERNAME for @" << username << std::endl;
    } else {
        std::cerr << "[BeaconClient] ✗ FAILED to send REGISTER_USERNAME!" << std::endl;
    }
    return sent;
}

bool BeaconClient::resolveUsername(const std::string& username, BeaconResponseCallback callback) {
    if (!isConfigured() || !m_transport) return false;
    auto data = createResolveUsernamePacket(username);
    m_pendingUsernameCallback = callback;
    m_waitingResponse = true;
    m_lastRequestTime = std::chrono::steady_clock::now();
    bool sent = m_transport->sendTo(m_beaconIp, m_beaconPort, std::string(data.begin(), data.end()));
    if (sent) {
        std::cout << "[BeaconClient] Sent RESOLVE_USERNAME for @" << username << std::endl;
    }
    return sent;
}

bool BeaconClient::requestBootstrap(const uint8_t myNodeId[20], BootstrapCallback callback) {
    if (!isConfigured() || !m_transport) return false;
    auto data = createBootstrapRequestPacket(myNodeId);
    m_pendingBootstrapCallback = callback;
    m_waitingResponse = true;
    m_lastRequestTime = std::chrono::steady_clock::now();
    bool sent = m_transport->sendTo(m_beaconIp, m_beaconPort, std::string(data.begin(), data.end()));
    if (sent) {
        std::cout << "[BeaconClient] Sent BOOTSTRAP_REQUEST" << std::endl;
    }
    return sent;
}

bool BeaconClient::pingBeacon() {
    if (!isConfigured() || !m_transport) return false;
    Packet pkt;
    pkt.type = MsgType::BEACON_HEARTBEAT;
    auto data = pkt.serialize();
    return m_transport->sendTo(m_beaconIp, m_beaconPort, std::string(data.begin(), data.end()));
}

void BeaconClient::handlePacket(const Packet& pkt, const std::string& fromIp, uint16_t fromPort) {
    if (fromIp != m_beaconIp || fromPort != m_beaconPort) return;
    switch (pkt.type) {
        case MsgType::USERNAME_RESPONSE: {
            std::cout << "[BeaconClient] <<< GOT USERNAME_RESPONSE" << std::endl;
            if (pkt.payload.size() < 1) {
                std::cerr << "[BeaconClient] Empty USERNAME_RESPONSE" << std::endl;
                return;
            }
            bool found = (pkt.payload[0] != 0);
            std::cout << "[BeaconClient] Result: " << (found ? "SUCCESS" : "FAILED") << std::endl;
            NodeInfo info{};
            if (found && pkt.payload.size() >= 159) {
                deserializeNodeInfo(pkt.payload.data() + 1, pkt.payload.size() - 1, info);
            }
            m_waitingResponse = false;
            if (m_pendingUsernameCallback) {
                std::cout << "[BeaconClient] Calling callback..." << std::endl;
                m_pendingUsernameCallback(found, info);
                m_pendingUsernameCallback = nullptr;
            } else {
                std::cout << "[BeaconClient] No pending callback!" << std::endl;
            }
            break;
        }
        case MsgType::BOOTSTRAP_RESPONSE: {
            if (pkt.payload.size() < 1) return;
            uint8_t count = pkt.payload[0];
            std::vector<NodeInfo> nodes;
            size_t offset = 1;
            for (int i = 0; i < count && offset + 158 <= pkt.payload.size(); ++i) {
                NodeInfo info{};
                if (deserializeNodeInfo(pkt.payload.data() + offset, 158, info)) {
                    nodes.push_back(info);
                }
                offset += 158;
            }
            m_waitingResponse = false;
            if (m_pendingBootstrapCallback) {
                m_pendingBootstrapCallback(nodes);
                m_pendingBootstrapCallback = nullptr;
            }
            break;
        }
        case MsgType::BEACON_STATUS: {
            std::cout << "[BeaconClient] Beacon is alive" << std::endl;
            break;
        }
        default:
            break;
    }
}

} // namespace hivemind