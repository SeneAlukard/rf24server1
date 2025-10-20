#pragma once

#include "packets.hpp"
#include "radio.hpp"
#include <mutex>
#include <string>
#include <vector>
#include <chrono>

struct GroundDroneInfo {
  DroneIdType id{};
  std::string name;
  float last_link_quality = 0.0f;
  std::chrono::steady_clock::time_point last_update;
  bool online = true;
};

class GroundBaseStation {
public:
  explicit GroundBaseStation(RadioInterface &radio); // use one radio for both TX and RX
  GroundBaseStation(RadioInterface &rx_radio, RadioInterface &tx_radio);

  void handleIncoming();
  void broadcastCommand(const std::string &cmd);
  void sendCommandToDrone(DroneIdType id, const std::string &cmd);
  std::vector<GroundDroneInfo> getDronesSnapshot() const;
  const std::vector<GroundDroneInfo> &getDrones() const { return drones_; }

private:
  RadioInterface &rx_radio_;
  RadioInterface &tx_radio_;
  std::vector<GroundDroneInfo> drones_;
  DroneIdType current_leader_ = 0;
  bool have_leader_ = false;
  mutable std::mutex drones_mutex_;

  void processJoinRequest(const JoinRequestPacket &req);
  void processTelemetry(const TelemetryPacket &tel);
};
