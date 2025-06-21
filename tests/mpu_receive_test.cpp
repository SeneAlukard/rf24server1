#include "radio.hpp"
#include "packets.hpp"
#include <chrono>
#include <iostream>
#include <thread>

#define RX_CE_PIN 22
#define RX_CSN_PIN 10

static constexpr uint64_t BASE_TX = 0xF0F0F0F0E1LL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0D2LL;

int main() {
    RadioInterface radio(RX_CE_PIN, RX_CSN_PIN);
    if (!radio.begin()) {
        std::cerr << "Radio init failed" << std::endl;
        return 1;
    }

    radio.configure(1, RadioDataRate::MEDIUM_RATE);
    radio.setAddress(BASE_TX, BASE_RX);

    std::cout << "Waiting for telemetry packets..." << std::endl;
    while (true) {
        PacketType type;
        if (radio.receive(&type, sizeof(type), true) && type == PacketType::TELEMETRY) {
            TelemetryPacket packet{};
            if (radio.receive(&packet, sizeof(packet))) {
                std::cout << "Accel: " << packet.acceleration_x << ',' << packet.acceleration_y
                          << ',' << packet.acceleration_z
                          << " Gyro: " << packet.gyroscope_x << ',' << packet.gyroscope_y
                          << ',' << packet.gyroscope_z
                          << " Altitude: " << packet.altitude
                          << " Battery: " << packet.battery_voltage
                          << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 0;
}
