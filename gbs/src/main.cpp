#include "gbs.hpp"
#include <chrono>
#include <iostream>
#include <thread>

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

  while (true) {
    gbs.handleIncoming();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
