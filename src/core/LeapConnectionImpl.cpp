#include "LeapConnectionImpl.hpp"
#include "core/Log.hpp"

LeapConnectionImpl::LeapConnectionImpl() {
    // Configure for multi-device awareness
    LEAP_CONNECTION_CONFIG config = {0};
    config.size = sizeof(config);
    config.flags = eLeapConnectionConfig_MultiDeviceAware;
    LEAP_CONNECTION rawHandle = nullptr;
    eLeapRS result = LeapCreateConnection(&config, &rawHandle); // Create into temp raw handle
    if (result != eLeapRS_Success || !rawHandle) {
        LOG_ERR("Failed to create Leap connection (result=" << result << ")");
        // No need to destroy rawHandle, LeapCreateConnection promises null on failure
        throw std::runtime_error("Failed to create Leap connection");
    }
    connectionHandle_.reset(rawHandle); // Take ownership with the handle wrapper
}

LeapConnectionImpl::~LeapConnectionImpl() {
    disconnect();
    // No explicit LeapDestroyConnection needed, HandleWrapper manages it.
}

bool LeapConnectionImpl::connect() {
    if (!connectionHandle_) return false; // Check the wrapper directly
    eLeapRS result = LeapOpenConnection(connectionHandle_.get()); // Use get() for the raw handle
    connected_ = (result == eLeapRS_Success);
    if (!connected_) {
        LOG_ERR("Failed to open Leap connection (result=" << result << ")");
    }
    return connected_;
}

void LeapConnectionImpl::disconnect() {
    // Check handle validity via the wrapper
    if (connectionHandle_ && connected_) {
        LeapCloseConnection(connectionHandle_.get()); // Use get() for the raw handle
        connected_ = false;
    }
}

bool LeapConnectionImpl::isConnected() const {
    // Check handle validity via the wrapper before checking connected flag
    return connectionHandle_ && connected_;
}

LEAP_CONNECTION LeapConnectionImpl::getRawConnection() const {
    return connectionHandle_.get(); // Use get() to return the raw handle
}
