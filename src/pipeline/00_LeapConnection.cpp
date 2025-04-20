#include "00_LeapConnection.hpp"
#include "core/Log.hpp"
#include <stdexcept>
#include "../../LeapSDK/include/LeapC.h" // Ensure LeapC.h is included for the flag
#include <windows.h> // Include for OutputDebugStringA

LeapConnection::LeapConnection() : connection_(nullptr) {
    OutputDebugStringA("LeapConnection: Constructor entered.\n");
    // Configure for multi-device awareness
    LEAP_CONNECTION_CONFIG config = {0}; // Initialize struct members to 0/null
    config.size = sizeof(config);
    config.flags = eLeapConnectionConfig_MultiDeviceAware;
    // config.server_namespace = nullptr; // Default namespace
    // config.tracking_origin = eLeapTrackingOrigin_DeviceCenter; // Default origin

    eLeapRS result = LeapCreateConnection(&config, &connection_); // Pass config struct
    if (result != eLeapRS_Success || !connection_) {
        LOG_ERR("Failed to create Leap connection (result=" << result << ")");
        throw std::runtime_error("Failed to create Leap connection");
    }
    result = LeapOpenConnection(connection_);
    if (result != eLeapRS_Success) {
        LOG_ERR("Failed to open Leap connection (result=" << result << ")");
        // Clean up created connection if open fails
        LeapDestroyConnection(connection_); 
        connection_ = nullptr;
        throw std::runtime_error("Failed to open Leap connection");
    }
    LOG("Leap connection created and opened successfully (Multi-Device Aware).");
    OutputDebugStringA("LeapConnection: Constructor exiting successfully.\n");
}

LeapConnection::~LeapConnection() {
    if (connection_) {
        LeapCloseConnection(connection_);
        LeapDestroyConnection(connection_);
        connection_ = nullptr;
    }
}

LEAP_CONNECTION LeapConnection::getConnection() const {
    return connection_;
}
