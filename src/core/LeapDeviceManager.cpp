#include "LeapDeviceManager.hpp"
#include "FrameData.hpp"
#include <algorithm>

void LeapDeviceManager::poll() {
    // Example: poll all connected devices and emit frames
    // For each device (pseudo-code):
    // for (const auto& device : devices_) {
    //     FrameData frame = ... // fetch from device
    //     for (auto& cb : callbacks_) {
    //         cb(device.id, frame);
    //     }
    // }
}

void LeapDeviceManager::registerFrameCallback(FrameCallback cb) {
    callbacks_.push_back(std::move(cb));
}
