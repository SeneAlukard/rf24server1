#pragma once

#include "packets.hpp"
#include "radio.hpp"
#include <mutex>
#include <string>
#include <vector>

struct GroundDroneInfo {
  DroneIdType id{};
  std::string name;
  float last_link_quality = 0.0f;
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
  mutable std::mutex drones_mutex_;

  void processJoinRequest(const JoinRequestPacket &req);
  void processTelemetry(const TelemetryPacket &tel);
};
