#pragma once
#include <string>
#include <cstdint>

// Event representing a device disconnection/loss
struct DeviceLostEvent {
    // uint32_t deviceId; // Removed
    std::string serialNumber;
    // Add more fields if needed (e.g., reason for loss, timestamp)
};
