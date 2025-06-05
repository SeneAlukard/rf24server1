#include "../include/radio.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>

RadioInterface::RadioInterface(uint8_t cePin, uint8_t csnPin)
    : radio(cePin, csnPin) {}

RadioInterface::RadioInterface(uint8_t cePin, uint8_t csnPin, uint8_t spiPort)
    : radio(cePin, csnPin, spiPort) {}

bool RadioInterface::begin() {
  if (!radio.begin())
    return false;
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
  return true;
}

void RadioInterface::setAddress(uint64_t tx, uint64_t rx) {
  tx_address = tx;
  rx_address = rx;
  radio.openWritingPipe(tx_address);
  radio.openReadingPipe(1, rx_address);
}

void RadioInterface::openListeningPipe(uint8_t pipe, uint64_t address) {
  radio.openReadingPipe(pipe, address);
}

void RadioInterface::configure(uint8_t channel, RadioDataRate datarate) {
  radio.setChannel(channel);
  switch (datarate) {
  case RadioDataRate::LOW_RATE:
    radio.setDataRate(RF24_250KBPS);
    break;
  case RadioDataRate::MEDIUM_RATE:
    radio.setDataRate(RF24_1MBPS);
    break;
  case RadioDataRate::HIGH_RATE:
    radio.setDataRate(RF24_2MBPS);
    break;
  }
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.startListening();
}

bool RadioInterface::send(const void *data, size_t size) {
  radio.stopListening();
  bool success = radio.write(data, static_cast<uint8_t>(size));
  radio.startListening();
  return success;
}

bool RadioInterface::receive(void *data, size_t size, bool peekOnly) {
  if (peekOnly) {
    if (cached_packet) {
      std::memcpy(data, cached_packet->data(), size);
      return true;
    }

    if (!radio.available())
      return false;

    cached_packet.emplace();
    radio.read(cached_packet->data(),
               static_cast<uint8_t>(cached_packet->size()));
    std::memcpy(data, cached_packet->data(), size);
    return true;
  }

  if (cached_packet) {
    std::memcpy(data, cached_packet->data(), size);
    cached_packet.reset();
    return true;
  }

  if (!radio.available())
    return false;

  std::array<uint8_t, 32> buffer{};
  radio.read(buffer.data(), buffer.size());
  std::memcpy(data, buffer.data(), size);
  cached_packet.reset();
  return true;
}

bool RadioInterface::testRPD() {
  return radio.testRPD();
}

uint8_t RadioInterface::getARC() {
  return radio.getARC();
}
