// src/network/PacketManager.cpp
#include "PacketManager.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>

namespace hivemind {

std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(1 + payload.size());
    result.push_back(static_cast<uint8_t>(type));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool Packet::deserialize(const uint8_t* data, size_t len, Packet& out) {
    if (len < 1) return false;
    out.type = static_cast<MsgType>(data[0]);
    out.payload.assign(data + 1, data + len);
    return true;
}

std::vector<uint8_t> serializeNodeInfo(const NodeInfo& info) {
    std::vector<uint8_t> result;
    result.reserve(158);
    result.insert(result.end(), info.nodeId, info.nodeId + 20);
    result.insert(result.end(), reinterpret_cast<const uint8_t*>(info.username),
                  reinterpret_cast<const uint8_t*>(info.username + 32));
    result.insert(result.end(), reinterpret_cast<const uint8_t*>(info.publicIp),
                  reinterpret_cast<const uint8_t*>(info.publicIp + 16));
    result.insert(result.end(), reinterpret_cast<const uint8_t*>(info.localIp),
                  reinterpret_cast<const uint8_t*>(info.localIp + 16));
    uint16_t port_net = htons(info.port);
    result.push_back((port_net >> 8) & 0xFF);
    result.push_back(port_net & 0xFF);
    uint64_t ts_net = htobe64(info.timestamp);
    for (int i = 7; i >= 0; --i) {
        result.push_back((ts_net >> (i * 8)) & 0xFF);
    }
    result.insert(result.end(), info.signature, info.signature + 64);
    return result;
}

bool deserializeNodeInfo(const uint8_t* data, size_t len, NodeInfo& out) {
    if (len < 158) return false;
    memcpy(out.nodeId, data, 20);
    memcpy(out.username, data + 20, 32);
    out.username[31] = '\0';
    memcpy(out.publicIp, data + 52, 16);
    out.publicIp[15] = '\0';
    memcpy(out.localIp, data + 68, 16);
    out.localIp[15] = '\0';
    out.port = ntohs((data[84] << 8) | data[85]);
    out.timestamp = be64toh(
        ((uint64_t)data[86] << 56) | ((uint64_t)data[87] << 48) |
        ((uint64_t)data[88] << 40) | ((uint64_t)data[89] << 32) |
        ((uint64_t)data[90] << 24) | ((uint64_t)data[91] << 16) |
        ((uint64_t)data[92] << 8)  | (uint64_t)data[93]
    );
    memcpy(out.signature, data + 94, 64);
    return true;
}

std::vector<uint8_t> createRegisterUsernamePacket(const std::string& username, const NodeInfo& info) {
    Packet pkt;
    pkt.type = MsgType::REGISTER_USERNAME;
    std::string userPadded = username;
    if (userPadded.length() > 31) userPadded = userPadded.substr(0, 31);
    userPadded.resize(32, '\0');
    pkt.payload.assign(userPadded.begin(), userPadded.end());
    auto nodeData = serializeNodeInfo(info);
    pkt.payload.insert(pkt.payload.end(), nodeData.begin(), nodeData.end());
    return pkt.serialize();
}

std::vector<uint8_t> createResolveUsernamePacket(const std::string& username) {
    Packet pkt;
    pkt.type = MsgType::RESOLVE_USERNAME;
    std::string userPadded = username;
    if (userPadded.length() > 31) userPadded = userPadded.substr(0, 31);
    userPadded.resize(32, '\0');
    pkt.payload.assign(userPadded.begin(), userPadded.end());
    return pkt.serialize();
}

std::vector<uint8_t> createUsernameResponsePacket(bool found, const NodeInfo& info) {
    Packet pkt;
    pkt.type = MsgType::USERNAME_RESPONSE;
    pkt.payload.push_back(found ? 1 : 0);
    if (found) {
        auto nodeData = serializeNodeInfo(info);
        pkt.payload.insert(pkt.payload.end(), nodeData.begin(), nodeData.end());
    }
    return pkt.serialize();
}

std::vector<uint8_t> createBootstrapRequestPacket(const uint8_t nodeId[20]) {
    Packet pkt;
    pkt.type = MsgType::BOOTSTRAP_REQUEST;
    pkt.payload.assign(nodeId, nodeId + 20);
    return pkt.serialize();
}

std::vector<uint8_t> createBootstrapResponsePacket(const std::vector<NodeInfo>& nodes) {
    Packet pkt;
    pkt.type = MsgType::BOOTSTRAP_RESPONSE;
    pkt.payload.push_back(static_cast<uint8_t>(nodes.size()));
    for (const auto& node : nodes) {
        auto data = serializeNodeInfo(node);
        pkt.payload.insert(pkt.payload.end(), data.begin(), data.end());
    }
    return pkt.serialize();
}

std::vector<uint8_t> createRequestUsersPackPacket(const std::string& requesterNodeId) {
    Packet pkt;
    pkt.type = MsgType::REQUEST_USERS_PACK;
    pkt.payload.assign(requesterNodeId.begin(), requesterNodeId.end());
    return pkt.serialize();
}

std::vector<uint8_t> createUsersPackResponsePacket(const std::vector<std::string>& packLines) {
    Packet pkt;
    pkt.type = MsgType::USERS_PACK_RESPONSE;
    // Формат: [количество строк][длина строки 1][строка 1]...
    pkt.payload.push_back(static_cast<uint8_t>(packLines.size()));
    for (const auto& line : packLines) {
        uint16_t len = static_cast<uint16_t>(line.length());
        pkt.payload.push_back((len >> 8) & 0xFF);
        pkt.payload.push_back(len & 0xFF);
        pkt.payload.insert(pkt.payload.end(), line.begin(), line.end());
    }
    return pkt.serialize();
}

std::vector<uint8_t> createRelayMessagePacket(const std::string& fromNodeId, 
                                               const std::string& toNodeId,
                                               const std::string& message) {
    Packet pkt;
    pkt.type = MsgType::RELAY_MESSAGE;
    // fromNodeId (40 bytes) + toNodeId (40 bytes) + message
    pkt.payload.assign(fromNodeId.begin(), fromNodeId.end());
    pkt.payload.insert(pkt.payload.end(), toNodeId.begin(), toNodeId.end());
    uint16_t msgLen = static_cast<uint16_t>(message.length());
    pkt.payload.push_back((msgLen >> 8) & 0xFF);
    pkt.payload.push_back(msgLen & 0xFF);
    pkt.payload.insert(pkt.payload.end(), message.begin(), message.end());
    return pkt.serialize();
}

} // namespace hivemind