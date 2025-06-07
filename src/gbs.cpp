#include "../include/gbs.hpp"
#include <array>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>

GroundBaseStation::GroundBaseStation(RadioInterface &radio) : radio_(radio) {}

void GroundBaseStation::processJoinRequest(const JoinRequestPacket &req) {
  std::lock_guard<std::mutex> lock(drones_mutex_);
  GroundDroneInfo info{};
  info.id = static_cast<DroneIdType>(drones_.size() + 1);
  info.name = req.requested_name;
  drones_.push_back(info);

  JoinResponsePacket resp{};
  resp.assigned_id = info.id;
  resp.current_leader_id = info.id; // first joiner becomes leader
  resp.assigned_channel = 2;        // fixed channel for demo
  resp.timestamp = static_cast<uint32_t>(std::time(nullptr));

  radio_.send(&resp, sizeof(resp));
  std::cout << "JoinRequest from " << info.name
            << " assigned ID " << static_cast<int>(info.id) << std::endl;
}

void GroundBaseStation::processTelemetry(const TelemetryPacket &tel) {
  std::lock_guard<std::mutex> lock(drones_mutex_);
  for (auto &d : drones_) {
    if (d.id == tel.drone_id) {
      d.last_link_quality = tel.link_quality;
      std::cout << "Telemetry from " << d.name
                << " LQ=" << tel.link_quality << std::endl;
      return;
    }
  }
}

void GroundBaseStation::handleIncoming() {
  PacketType peek;
  while (radio_.receive(&peek, sizeof(PacketType), true)) {
    switch (peek) {
    case PacketType::JOIN_REQUEST: {
      JoinRequestPacket req{};
      if (radio_.receive(&req, sizeof(req)))
        processJoinRequest(req);
      break;
    }
    case PacketType::TELEMETRY: {
      TelemetryPacket tel{};
      if (radio_.receive(&tel, sizeof(tel)))
        processTelemetry(tel);
      break;
    }
    default: {
      std::array<uint8_t, 32> dummy{};
      radio_.receive(dummy.data(), dummy.size());
      break;
    }
    }
  }
}

void GroundBaseStation::broadcastCommand(const std::string &cmd) {
  CommandPacket packet{};
  packet.timestamp = static_cast<uint32_t>(std::time(nullptr));
  std::strncpy(packet.command, cmd.c_str(), MAX_COMMAND_LENGTH - 1);
  packet.command[MAX_COMMAND_LENGTH - 1] = '\0';

  std::lock_guard<std::mutex> lock(drones_mutex_);
  for (const auto &d : drones_) {
    packet.target_drone_id = d.id;
    radio_.send(&packet, sizeof(packet));
  }
}

void GroundBaseStation::sendCommandToDrone(DroneIdType id,
                                           const std::string &cmd) {
  CommandPacket packet{};
  packet.timestamp = static_cast<uint32_t>(std::time(nullptr));
  packet.target_drone_id = id;
  std::strncpy(packet.command, cmd.c_str(), MAX_COMMAND_LENGTH - 1);
  packet.command[MAX_COMMAND_LENGTH - 1] = '\0';

  radio_.send(&packet, sizeof(packet));
}

std::vector<GroundDroneInfo> GroundBaseStation::getDronesSnapshot() const {
  std::lock_guard<std::mutex> lock(drones_mutex_);
  return drones_;
}
