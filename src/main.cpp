#include "gbs.hpp"
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#define CE_PIN 27
#define CSN_PIN 10

static constexpr uint64_t BASE_TX = 0xF0F0F0F0E1LL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0D2LL;

int main() {
  RadioInterface radio(CE_PIN, CSN_PIN);
  if (!radio.begin()) {
    std::cerr << "Radio init failed" << std::endl;
    return 1;
  }

  radio.configure(1, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  GroundBaseStation gbs(radio);

  std::mutex cmd_mutex;
  std::queue<std::pair<DroneIdType, std::string>> cmd_queue;

  std::thread input_thread([&]() {
    while (true) {
      auto drones = gbs.getDronesSnapshot();
      std::cout << "\nConnected drones:" << std::endl;
      for (const auto &d : drones) {
        std::cout << "  ID " << static_cast<int>(d.id) << " - " << d.name
                  << std::endl;
      }
      std::cout << "Enter target drone id (0=all, empty to skip): ";
      std::string id_line;
      if (!std::getline(std::cin, id_line))
        break;
      if (id_line.empty())
        continue;
      DroneIdType id = static_cast<DroneIdType>(std::stoi(id_line));
      std::cout << "Enter command: ";
      std::string cmd;
      if (!std::getline(std::cin, cmd))
        break;
      if (cmd.empty())
        continue;
      std::lock_guard<std::mutex> lock(cmd_mutex);
      cmd_queue.emplace(id, cmd);
    }
  });
  input_thread.detach();

  while (true) {
    gbs.handleIncoming();
    {
      std::lock_guard<std::mutex> lock(cmd_mutex);
      if (!cmd_queue.empty()) {
        auto [id, cmd] = cmd_queue.front();
        cmd_queue.pop();
        if (id == 0)
          gbs.broadcastCommand(cmd);
        else
          gbs.sendCommandToDrone(id, cmd);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
