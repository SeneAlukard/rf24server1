#pragma once
#include <cstdint>

inline float accelRawToMs2(int16_t raw) {
    // Convert raw accelerometer reading to m/s^2
    return static_cast<float>(raw) / 16384.0f * 9.81f;
}

inline float gyroRawToDps(int16_t raw) {
    // Convert raw gyro reading to degrees/s
    return static_cast<float>(raw) / 131.0f;
}
