#include "../include/gbs.hpp"
#include <array>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <chrono>
#include <algorithm>

static constexpr auto OFFLINE_TIMEOUT = std::chrono::seconds(2);
GroundBaseStation::GroundBaseStation(RadioInterface &radio)
    : rx_radio_(radio), tx_radio_(radio) {}

GroundBaseStation::GroundBaseStation(RadioInterface &rx_radio,
                                     RadioInterface &tx_radio)
    : rx_radio_(rx_radio), tx_radio_(tx_radio) {}

void GroundBaseStation::processJoinRequest(const JoinRequestPacket &req) {
  std::lock_guard<std::mutex> lock(drones_mutex_);
  GroundDroneInfo info{};
  info.id = static_cast<DroneIdType>(drones_.size() + 1);
  info.name = req.requested_name;
  info.last_update = std::chrono::steady_clock::now();
  info.online = true;
  drones_.push_back(info);

  if (drones_.size() == 1) {
    current_leader_ = info.id;
    have_leader_ = true;
  }

  JoinResponsePacket resp{};
  resp.assigned_id = info.id;
  resp.current_leader_id = info.id; // first joiner becomes leader
  resp.assigned_channel = 2;        // fixed channel for demo
  resp.timestamp = static_cast<uint32_t>(std::time(nullptr));

  tx_radio_.send(&resp, sizeof(resp));
  std::cout << "JoinRequest from " << info.name
            << " assigned ID " << static_cast<int>(info.id) << std::endl;
}

void GroundBaseStation::processTelemetry(const TelemetryPacket &tel) {
  std::lock_guard<std::mutex> lock(drones_mutex_);
  for (auto &d : drones_) {
    if (d.id == tel.drone_id) {
      d.last_link_quality = tel.link_quality;
      d.last_update = std::chrono::steady_clock::now();
      if (!d.online) {
        d.online = true;
        std::cout << "Drone " << static_cast<int>(d.id) << " online" << std::endl;
      }
      std::cout << "Telemetry from " << d.name
                << " LQ=" << tel.link_quality << std::endl;
      return;
    }
  }
}

void GroundBaseStation::handleIncoming() {
  PacketType peek;
  while (rx_radio_.receive(&peek, sizeof(PacketType), true)) {
    switch (peek) {
    case PacketType::JOIN_REQUEST: {
      JoinRequestPacket req{};
      if (rx_radio_.receive(&req, sizeof(req)))
        processJoinRequest(req);
      break;
    }
    case PacketType::TELEMETRY: {
      TelemetryPacket tel{};
      if (rx_radio_.receive(&tel, sizeof(tel)))
        processTelemetry(tel);
      break;
    }
    default: {
      std::array<uint8_t, 32> dummy{};
      rx_radio_.receive(dummy.data(), dummy.size());
      break;
    }
    }
  }

  auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(drones_mutex_);

  for (auto &d : drones_) {
    bool is_online = now - d.last_update <= OFFLINE_TIMEOUT;
    if (d.online != is_online) {
      d.online = is_online;
      std::cout << "Drone " << static_cast<int>(d.id)
                << (is_online ? " online" : " offline") << std::endl;
    }
  }

  if (have_leader_) {
    auto it = std::find_if(drones_.begin(), drones_.end(),
                           [&](const GroundDroneInfo &di) {
                             return di.id == current_leader_;
                           });
    if (it == drones_.end() || !it->online) {
      have_leader_ = false;
    }
  }

  if (!have_leader_) {
    bool found = false;
    DroneIdType best_id = 0;
    auto best_time = std::chrono::steady_clock::time_point::min();
    for (const auto &d : drones_) {
      if (!d.online)
        continue;
      if (!found || d.last_update > best_time) {
        found = true;
        best_id = d.id;
        best_time = d.last_update;
      }
    }
    if (found) {
      current_leader_ = best_id;
      have_leader_ = true;

      LeaderAnnouncementPacket pkt{};
      pkt.new_leader_id = current_leader_;
      pkt.timestamp = static_cast<uint32_t>(std::time(nullptr));
      tx_radio_.send(&pkt, sizeof(pkt));
      uint8_t arc = tx_radio_.getARC();

      std::cout << "New leader: " << static_cast<int>(current_leader_)
                << " ARC: " << static_cast<int>(arc) << std::endl;
    } else {
      std::cout << "No leader" << std::endl;
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
    tx_radio_.send(&packet, sizeof(packet));
  }
}

void GroundBaseStation::sendCommandToDrone(DroneIdType id,
                                           const std::string &cmd) {
  CommandPacket packet{};
  packet.timestamp = static_cast<uint32_t>(std::time(nullptr));
  packet.target_drone_id = id;
  std::strncpy(packet.command, cmd.c_str(), MAX_COMMAND_LENGTH - 1);
  packet.command[MAX_COMMAND_LENGTH - 1] = '\0';

  tx_radio_.send(&packet, sizeof(packet));
}

std::vector<GroundDroneInfo> GroundBaseStation::getDronesSnapshot() const {
  std::lock_guard<std::mutex> lock(drones_mutex_);
  return drones_;
}
