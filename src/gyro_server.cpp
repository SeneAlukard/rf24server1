#include "radio.hpp"
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <algorithm>

#define TX_CE_PIN 27
#define TX_CSN_PIN 0
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
  bool strong_signal;
  std::chrono::steady_clock::time_point last_update;
};

int main() {
  RadioInterface txRadio(TX_CE_PIN, TX_CSN_PIN);
  RadioInterface rxRadio(RX_CE_PIN, RX_CSN_PIN);
  if (!txRadio.begin() || !rxRadio.begin()) {
    std::cerr << "Radio init failed" << std::endl;
    return 1;
  }

  txRadio.configure(1, RadioDataRate::MEDIUM_RATE);
  rxRadio.configure(1, RadioDataRate::MEDIUM_RATE);
  txRadio.setAddress(BASE_TX, BASE_RX);
  rxRadio.setAddress(BASE_TX, BASE_RX);

  std::unordered_map<uint8_t, GyroData> log;
  uint8_t current_leader = 0;
  bool have_leader = false;

  std::cout << "Listening for packets..." << std::endl;
  while (true) {
    SimplePacket pkt{};
    if (rxRadio.receive(&pkt, sizeof(pkt))) {
      GyroData &data = log[pkt.drone_id];
      data.gx = pkt.gyro_x;
      data.gy = pkt.gyro_y;
      data.gz = pkt.gyro_z;
      data.leader = pkt.leader;
      data.strong_signal = rxRadio.testRPD();
      data.last_update = std::chrono::steady_clock::now();

      std::cout << "ID " << static_cast<int>(pkt.drone_id)
                << " Gyro: " << pkt.gyro_x << ',' << pkt.gyro_y << ','
                << pkt.gyro_z
                << " Leader: " << std::boolalpha << pkt.leader
                << " RSSI> -64: " << data.strong_signal
                << std::endl;
    }

    auto now = std::chrono::steady_clock::now();
    if (have_leader) {
      auto it = log.find(current_leader);
      if (it == log.end() ||
          now - it->second.last_update > std::chrono::seconds(2)) {
        have_leader = false;
      }
    }
    if (!have_leader && !log.empty()) {
      auto it = std::max_element(
          log.begin(), log.end(),
          [](const auto &a, const auto &b) {
            return a.second.last_update < b.second.last_update;
          });
      current_leader = it->first;
      have_leader = true;

      char msg[32]{};
      std::snprintf(msg, sizeof(msg), "%u Leader = True", current_leader);
      txRadio.send(msg, sizeof(msg));

      std::cout << "New leader: " << static_cast<int>(current_leader)
                << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}