#pragma once

#include "packets.hpp"
#include "radio.hpp"
#include <string>
#include <vector>

struct GroundDroneInfo {
  DroneIdType id{};
  std::string name;
  float last_link_quality = 0.0f;
};

class GroundBaseStation {
public:
  explicit GroundBaseStation(RadioInterface &radio);

  void handleIncoming();
  void broadcastCommand(const std::string &cmd);
  const std::vector<GroundDroneInfo> &getDrones() const { return drones_; }

private:
  RadioInterface &radio_;
  std::vector<GroundDroneInfo> drones_;

  void processJoinRequest(const JoinRequestPacket &req);
  void processTelemetry(const TelemetryPacket &tel);
};
