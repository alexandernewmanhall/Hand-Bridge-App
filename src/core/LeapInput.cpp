#include "LeapInput.hpp"
#include "FrameData.hpp"
#include <chrono>
#include <thread>
#include "../utils/SpscQueue.hpp"
#include <windows.h>
#include <memory>

LeapInput::LeapInput(LEAP_CONNECTION connection, std::shared_ptr<SpscQueue<FrameData>> queue)
    : poller_(std::make_unique<LeapPoller>(connection))
    , frameQueue_(std::move(queue))
{
    // Add logging here *before* the check
    OutputDebugStringA(frameQueue_ ? "LeapInput: Received VALID queue shared_ptr.\n" : "LeapInput: Received NULL queue shared_ptr!\n");

    if (!frameQueue_) {
        // Handle error: queue pointer cannot be null
        throw std::invalid_argument("LeapInput: SpscQueue shared_ptr cannot be null.");
    }
    if (onDeviceConnected_) poller_->setDeviceConnectedCallback(onDeviceConnected_);
    if (onDeviceLost_) poller_->setDeviceLostCallback(onDeviceLost_);
    // Register this LeapInput as the callback target for connect/disconnect
    poller_->setLeapInputCallback(this);
}

void LeapInput::setFrameCallback(FrameCallback cb) {
    highLevelCallback_ = std::move(cb);
}

// IFrameSource implementation
bool LeapInput::getNextFrame(FrameData& outFrame) {
    std::lock_guard<std::mutex> lock(latestFrameMutex_);
    if (!hasFrame_) return false;
    outFrame = latestFrame_;
    return true;
}

void LeapInput::start() {
    if (poller_) {
        poller_->initializeDevices();
        poller_->setFrameCallback([this, localFrameQueue = this->frameQueue_](const FrameData& frameData) {
            // Store latest frame for IFrameSource (optional, depends if needed)
            {
                std::lock_guard<std::mutex> lock(latestFrameMutex_);
                latestFrame_ = frameData;
                hasFrame_ = true;
            }

            // Log that the callback is attempting to push
            OutputDebugStringA(("LeapInput frame callback invoked for SN: " + frameData.deviceId + ". Attempting to push to queue.\n").c_str());

            // --- Push frame data onto the SPSC queue using the captured shared_ptr --- 
            if (localFrameQueue) { // Check captured pointer validity
                if (!localFrameQueue->try_push(frameData)) { // Push a copy
                     // Log or handle queue full condition
                     OutputDebugStringA("Warning: Leap SPSC frame queue full. Frame dropped.\n");
                }
            } else {
                 // This case should ideally not happen if initialization is correct
                 OutputDebugStringA("Error: LeapInput frame callback lambda has null queue pointer!\n");
            }
        });
    }
    running_ = true;
    OutputDebugStringA("LeapInput::start() - Creating poll thread...\n");
    pollThread_ = std::thread([this]() { pollLoop(); });
    OutputDebugStringA("LeapInput::start() - Poll thread created.\n");
}

void LeapInput::stop() {
    // Signal the loop to stop
    running_ = false;

    if (pollThread_.joinable()) {
        pollThread_.join();
    }
}

LeapInput::~LeapInput() {
    stop();
}

void LeapInput::emitTestFrame(const std::string& deviceId, const FrameData& frame) {
    // Not implemented: test frame emission
}


void LeapInput::setDeviceConnectedCallback(LeapPoller::DeviceConnectedCallback cb) {
    onDeviceConnected_ = std::move(cb);
    if (poller_) poller_->setDeviceConnectedCallback(onDeviceConnected_);
}

void LeapInput::setDeviceLostCallback(LeapPoller::DeviceLostCallback cb) {
    onDeviceLost_ = std::move(cb);
    if (poller_) poller_->setDeviceLostCallback(onDeviceLost_);
}

void LeapInput::setConnectCallback(ConnectCallback cb) {
    onConnect_ = std::move(cb);
}

void LeapInput::setDisconnectCallback(DisconnectCallback cb) {
    onDisconnect_ = std::move(cb);
}

void LeapInput::pollLoop() {
    OutputDebugStringA("LeapInput::pollLoop() - Thread started.\n");
    while (running_.load()) {
        OutputDebugStringA("LeapInput::pollLoop() - Loop iteration start.\n");
        if (poller_) {
            OutputDebugStringA("LeapInput::pollLoop() - poller_ is valid, about to call poll().\n");
            OutputDebugStringA("LeapInput::pollLoop() - Calling poller_->poll()...\n");
            poller_->poll();
        } else {
            OutputDebugStringA("LeapInput::pollLoop() - poller_ is NULL!\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // Yield/Sleep to prevent busy-waiting and high CPU usage
        // std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    OutputDebugStringA("LeapInput::pollLoop() - Thread exiting.\n");
}

// These would be called by Leap event handlers. You need to call them from the relevant Leap events.
void LeapInput::onLeapServiceConnect() {
    if (onConnect_) {
        ConnectEvent event;
        onConnect_(event);
    }
}

void LeapInput::onLeapServiceDisconnect() {
    if (onDisconnect_) {
        DisconnectEvent event;
        onDisconnect_(event);
    }
}

