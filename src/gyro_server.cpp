#include "radio.hpp"
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <optional>
#include <thread>

#define RX_CE_PIN 22
#define RX_CSN_PIN 10
static constexpr uint64_t BASE_TX = 0xF0F0F0F0E1ULL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0D2ULL;

struct SimplePacket {
    uint8_t drone_id;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    bool leader;
};

struct GyroData {
  int16_t gx;
  int16_t gy;
  int16_t gz;
  bool leader;
  std::chrono::steady_clock::time_point last_update;
};

int main() {
  RadioInterface radio(RX_CE_PIN, RX_CSN_PIN);
  if (!radio.begin()) {
    std::cerr << "Radio init failed" << std::endl;
    return 1;
  }

  radio.configure(1, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  std::unordered_map<uint8_t, GyroData> log;
  std::optional<uint8_t> current_leader;

  std::cout << "Listening for packets..." << std::endl;
  while (true) {
    SimplePacket pkt{};
    if (radio.receive(&pkt, sizeof(pkt))) {
      GyroData &data = log[pkt.drone_id];
      data.gx = pkt.gyro_x;
      data.gy = pkt.gyro_y;
      data.gz = pkt.gyro_z;
      data.leader = pkt.leader;
      data.last_update = std::chrono::steady_clock::now();

      std::cout << "ID " << static_cast<int>(pkt.drone_id)
                << " Gyro: " << pkt.gyro_x << ',' << pkt.gyro_y << ','
                << pkt.gyro_z
                << " Leader: " << std::boolalpha << pkt.leader
                << std::endl;
    }

    auto now = std::chrono::steady_clock::now();
    if (current_leader) {
      auto it = log.find(*current_leader);
      if (it == log.end() ||
          now - it->second.last_update > std::chrono::seconds(2)) {
        current_leader.reset();
      }
    }
    if (!current_leader && !log.empty()) {
      auto it = std::max_element(
          log.begin(), log.end(),
          [](const auto &a, const auto &b) {
            return a.second.last_update < b.second.last_update;
          });
      current_leader = it->first;

      char msg[32]{};
      std::snprintf(msg, sizeof(msg), "%u Leader = True", *current_leader);
      radio.send(msg, sizeof(msg));

      std::cout << "New leader: " << static_cast<int>(*current_leader)
                << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}
