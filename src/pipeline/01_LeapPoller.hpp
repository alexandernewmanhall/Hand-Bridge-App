#pragma once
#include "LeapC.h"
#include <string>
#include <vector>
#include <functional>
#include "../core/FrameData.hpp"

// 01_LeapPoller: Handles Leap device polling and emits tracking events
#include <atomic>
#include <thread>

class LeapPoller {
public:
    // Interface for LeapInput callbacks
    struct LeapInputCallback {
        virtual void onLeapServiceConnect() = 0;
        virtual void onLeapServiceDisconnect() = 0;
        virtual ~LeapInputCallback() = default;
    };

public:
    struct DeviceInfo {
        uint32_t id; // Unique device id from LeapC
        LEAP_DEVICE deviceHandle;
        std::string serialNumber;
        DeviceInfo() : id(0), deviceHandle(nullptr) {}
    };


    using FrameCallback = std::function<void(const FrameData& frame)>;

    LeapPoller(LEAP_CONNECTION connection);
    ~LeapPoller();

    bool initializeDevices();
    void handleDeviceEvent(const LEAP_DEVICE_EVENT* deviceEvent);
    void handleDeviceLost(const LEAP_DEVICE_EVENT* deviceEvent);
    void handleTracking(const LEAP_TRACKING_EVENT* tracking, const std::string& serialNumber);
    void poll();

    const std::vector<DeviceInfo>& getDevices() const;
    void setFrameCallback(FrameCallback cb);
    void cleanup();

public:
    using DeviceConnectedCallback = std::function<void(const DeviceInfo&)>;
    using DeviceLostCallback = std::function<void(const std::string& serialNumber)>;
    void setDeviceConnectedCallback(DeviceConnectedCallback cb) { onDeviceConnected_ = std::move(cb); }
    void setDeviceLostCallback(DeviceLostCallback cb) { onDeviceLost_ = std::move(cb); }

public:
    void setLeapInputCallback(LeapInputCallback* cb) { leapInputCallback_ = cb; }

private:
    LEAP_CONNECTION connection_;
    std::vector<DeviceInfo> devices_;
    FrameCallback frameCallback_;
    DeviceConnectedCallback onDeviceConnected_;
    DeviceLostCallback onDeviceLost_;
    LeapInputCallback* leapInputCallback_ = nullptr;
    bool getDeviceSerial(LEAP_DEVICE device, std::string& serialNumber);
    static const char* GetLeapRSString(eLeapRS code);
};
