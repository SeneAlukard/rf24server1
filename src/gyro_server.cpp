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
static constexpr auto OFFLINE_TIMEOUT = std::chrono::seconds(2);

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
  bool online = true;
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
      data.online = true;

      std::cout << "ID " << static_cast<int>(pkt.drone_id)
                << " Gyro: " << pkt.gyro_x << ',' << pkt.gyro_y << ','
                << pkt.gyro_z
                << " Leader: " << std::boolalpha << pkt.leader
                << " RSSI> -64: " << data.strong_signal
                << std::endl;
    }

    auto now = std::chrono::steady_clock::now();

    for (auto &[id, data] : log) {
      bool is_online = now - data.last_update <= OFFLINE_TIMEOUT;
      if (data.online != is_online) {
        data.online = is_online;
        std::cout << "Drone " << static_cast<int>(id)
                  << (is_online ? " online" : " offline") << std::endl;
      }
    }

    if (have_leader) {
      auto it = log.find(current_leader);
      if (it == log.end() || !it->second.online) {
        have_leader = false;
      }
    }

    if (!have_leader) {
      bool found = false;
      uint8_t best_id = 0;
      auto best_time = std::chrono::steady_clock::time_point::min();
      for (const auto &kv : log) {
        if (!kv.second.online)
          continue;
        if (!found || kv.second.last_update > best_time) {
          found = true;
          best_id = kv.first;
          best_time = kv.second.last_update;
        }
      }
      if (found) {
        current_leader = best_id;
        have_leader = true;

        char msg[32]{};
        std::snprintf(msg, sizeof(msg), "%u Leader = True", current_leader);
        txRadio.send(msg, sizeof(msg));
        uint8_t arc = txRadio.getARC();

        std::cout << "New leader: " << static_cast<int>(current_leader)
                  << " ARC: " << static_cast<int>(arc)
                  << std::endl;
      } else {
        std::cout << "No leader" << std::endl;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}
