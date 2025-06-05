#pragma once

#include <cstddef>
#include <cstdint>

using DroneIdType = uint8_t;
// ==================== Packet Types ==================== //

enum class PacketType : uint8_t {
  UNDEFINED = 0,
  JOIN_REQUEST = 1,
  JOIN_RESPONSE = 2,
  COMMAND = 3,
  TELEMETRY = 4,
  HEARTBEAT = 5,
  LEADER_ANNOUNCEMENT = 6,
  PERMISSION_TO_SEND = 7,
  LEADER_REQUEST = 8,
};
// ==================== Constants ==================== //

constexpr size_t MAX_COMMAND_LENGTH = 20;
constexpr size_t MAX_NODE_NAME_LENGTH = 20;

// ==================== Packet Structures ==================== //

#pragma pack(push, 1)
struct CommandPacket {
  PacketType type = PacketType::COMMAND;
  DroneIdType target_drone_id;
  uint32_t timestamp;
  char command[MAX_COMMAND_LENGTH];
};

struct TelemetryPacket {
  PacketType type = PacketType::TELEMETRY;
  DroneIdType drone_id;
  uint32_t timestamp;
  int16_t acceleration_x, acceleration_y, acceleration_z;
  int16_t gyroscope_x, gyroscope_y, gyroscope_z;
  float battery_voltage;
  float altitude;
  uint8_t rpd;
  uint8_t retries;
  float link_quality;
};

struct JoinRequestPacket {
  PacketType type = PacketType::JOIN_REQUEST;
  uint32_t timestamp;
  DroneIdType temp_id;
  char requested_name[MAX_NODE_NAME_LENGTH];
};

struct JoinResponsePacket {
  PacketType type = PacketType::JOIN_RESPONSE;
  DroneIdType assigned_id;
  DroneIdType current_leader_id;
  uint8_t assigned_channel;
  uint32_t timestamp;
};

struct HeartbeatPacket {
  PacketType type = PacketType::HEARTBEAT;
  DroneIdType source_drone_id;
  uint32_t timestamp;
};

struct LeaderAnnouncementPacket {
  PacketType type = PacketType::LEADER_ANNOUNCEMENT;
  DroneIdType new_leader_id;
  uint32_t timestamp;
};

struct PermissionToSendPacket {
  PacketType type = PacketType::PERMISSION_TO_SEND;
  DroneIdType target_drone_id;
  uint32_t timestamp;
};

struct LeaderRequestPacket {
  PacketType type = PacketType::LEADER_REQUEST;
  DroneIdType drone_id;
  uint32_t timestamp;
};
#pragma pack(pop)

// ==================== Assertions for Packet Sizes ==================== //
static_assert(sizeof(LeaderAnnouncementPacket) == 6,
              "LeaderAnnouncementPacket size mismatch");

static_assert(sizeof(HeartbeatPacket) == 6, "HeartbeatPacket size mismatch");

static_assert(sizeof(JoinResponsePacket) == 8,
              "JoinResponsePacket boyutu hatalı");

static_assert(sizeof(JoinRequestPacket) == 26,
              "JoinRequestPacket boyutu hatalı");

static_assert(sizeof(TelemetryPacket) == 32, "TelemetryPacket size mismatch");

static_assert(sizeof(CommandPacket) == 26, "CommandPacket size mismatch");

static_assert(sizeof(PermissionToSendPacket) == 6,
              "PermissionToSendPacket size mismatch");

static_assert(sizeof(LeaderRequestPacket) == 6,
              "LeaderRequestPacket size mismatch");
