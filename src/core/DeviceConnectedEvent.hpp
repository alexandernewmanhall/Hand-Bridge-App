#pragma once
#include <string>
#include <cstdint>

// Event representing a device connection
struct DeviceConnectedEvent {
    // uint32_t deviceId; // Ensure removed or commented out
    std::string serialNumber;
    // Add more fields if needed (e.g., device name, connection time)
};
