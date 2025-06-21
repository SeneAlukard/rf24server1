#include "radio.hpp"
#include <chrono>
#include <iostream>
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

int main() {
    RadioInterface radio(RX_CE_PIN, RX_CSN_PIN);
    if (!radio.begin()) {
        std::cerr << "Radio init failed" << std::endl;
        return 1;
    }

    radio.configure(1, RadioDataRate::MEDIUM_RATE);
    radio.setAddress(BASE_TX, BASE_RX);

    std::cout << "Listening for packets..." << std::endl;
    while (true) {
        SimplePacket pkt{};
        if (radio.receive(&pkt, sizeof(pkt))) {
            std::cout << "ID " << static_cast<int>(pkt.drone_id)
                      << " Gyro: " << pkt.gyro_x << ',' << pkt.gyro_y << ','
                      << pkt.gyro_z
                      << " Leader: " << std::boolalpha << pkt.leader
                      << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}
