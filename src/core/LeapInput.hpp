#pragma once
#include "IFrameStreamingInputDevice.hpp"
#include "FrameCallback.hpp"
#include "ConnectEvent.hpp"
#include "DisconnectEvent.hpp"
#include "FrameData.hpp"
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <memory>

#include "../pipeline/01_LeapPoller.hpp"

#include "interfaces/IFrameSource.hpp"

#include "../utils/SpscQueue.hpp"

class LeapInput : public IFrameStreamingInputDevice, public LeapPoller::LeapInputCallback, public IFrameSource {
public:
    using ConnectCallback = std::function<void(const ConnectEvent&)>;
    using DisconnectCallback = std::function<void(const DisconnectEvent&)>;
public:
    // Use FrameCallback from FrameCallback.hpp
    // Use DeviceInfo directly from LeapPoller
    using DeviceConnectedCallback = LeapPoller::DeviceConnectedCallback;
    // Use the updated signature from LeapPoller
    using DeviceLostCallback = LeapPoller::DeviceLostCallback;

    LeapInput(LEAP_CONNECTION connection, std::shared_ptr<SpscQueue<FrameData>> queue);
    // Update signature to match IInputDevice and mark override
    void setFrameCallback(FrameCallback cb) override;
    void start() override;
    void stop() override;
    ~LeapInput() override;

    // For testing: inject mock data
    void emitTestFrame(const std::string& deviceId, const FrameData& frame); // Not implemented

    // LeapPoller::LeapInputCallback interface
    void onLeapServiceConnect() override;
    void onLeapServiceDisconnect() override;


public:
    void setDeviceConnectedCallback(DeviceConnectedCallback cb) override;
    void setDeviceLostCallback(DeviceLostCallback cb) override;
    void setConnectCallback(ConnectCallback cb);
    void setDisconnectCallback(DisconnectCallback cb);
private:
    std::unique_ptr<LeapPoller> poller_;
    FrameCallback highLevelCallback_;
    std::shared_ptr<SpscQueue<FrameData>> frameQueue_;
    DeviceConnectedCallback onDeviceConnected_;
    DeviceLostCallback onDeviceLost_;
    std::atomic<bool> running_{false};
    std::thread pollThread_;
    void pollLoop();
    ConnectCallback onConnect_;
    DisconnectCallback onDisconnect_;
    // For IFrameSource
    std::mutex latestFrameMutex_;
    FrameData latestFrame_;
    bool hasFrame_ = false;
public:
    // IFrameSource implementation
    bool getNextFrame(FrameData& outFrame) override;
};
