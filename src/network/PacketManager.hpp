// include/hivemind/PacketManager.hpp
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <functional>

namespace hivemind {

enum class MsgType : uint8_t {
    MESSAGE = 0,
    PING = 1,
    PONG = 2,
    REGISTER_USERNAME = 10,
    RESOLVE_USERNAME = 11,
    USERNAME_RESPONSE = 12,
    BOOTSTRAP_REQUEST = 20,
    BOOTSTRAP_RESPONSE = 21,
    BEACON_HEARTBEAT = 30,
    BEACON_STATUS = 31,
    
    // === НОВЫЕ ТИПЫ ДЛЯ РЕТРАНСЛЯЦИИ ===
    REQUEST_USERS_PACK = 40,      // Запросить users.pack
    USERS_PACK_RESPONSE = 41,     // Ответ с users.pack
    RELAY_MESSAGE = 50,           // Ретранслируемое сообщение
    RELAY_DELIVERY = 51,          // Подтверждение доставки ретрансляции
};

struct Packet {
    MsgType type;
    std::vector<uint8_t> payload;
    
    std::vector<uint8_t> serialize() const;
    static bool deserialize(const uint8_t* data, size_t len, Packet& out);
};

struct NodeInfo {
    uint8_t nodeId[20];
    char username[32];
    char publicIp[16];
    char localIp[16];
    uint16_t port;
    uint64_t timestamp;
    uint8_t signature[64];
};

std::vector<uint8_t> serializeNodeInfo(const NodeInfo& info);
bool deserializeNodeInfo(const uint8_t* data, size_t len, NodeInfo& out);

std::vector<uint8_t> createRegisterUsernamePacket(const std::string& username, const NodeInfo& info);
std::vector<uint8_t> createResolveUsernamePacket(const std::string& username);
std::vector<uint8_t> createUsernameResponsePacket(bool found, const NodeInfo& info);
std::vector<uint8_t> createBootstrapRequestPacket(const uint8_t nodeId[20]);
std::vector<uint8_t> createBootstrapResponsePacket(const std::vector<NodeInfo>& nodes);

std::vector<uint8_t> createRequestUsersPackPacket(const std::string& requesterNodeId);
std::vector<uint8_t> createUsersPackResponsePacket(const std::vector<std::string>& packLines);
std::vector<uint8_t> createRelayMessagePacket(const std::string& fromNodeId, 
                                               const std::string& toNodeId,
                                               const std::string& message);

} // namespace hivemind