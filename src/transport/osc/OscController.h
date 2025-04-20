#pragma once
#include "transport/osc/OscMessage.hpp"
#include "core/FrameData.hpp"
#include "core/DeviceHandAssignedEvent.hpp"
#include "core/ConnectEvent.hpp"
#include <mutex>
#include <map>
#include <string>
#include <atomic>
#include <map>
#include "core/interfaces/IConfigStore.hpp"
#include <optional>
#include <memory>
#include "core/interfaces/ITransportSink.hpp"
#include <functional>
#include "core/ThreadSafeQueue.hpp"
#include <thread>
#include "core/AppLogger.hpp"
// Forward declarations
class MainAppWindow;
class ITransportSink;
struct HandData;
struct FrameData;
struct FrameDataEvent;
class ConfigManagerInterface;
class AppLogger;

class OscController {
public:
    using EnableOscCallback = std::function<void(bool)>;
    OscController(ITransportSink& sender, ConfigManagerInterface& configMgr, std::shared_ptr<AppLogger> logger);

    ~OscController() = default;
    OscController(const OscController&) = delete;
    OscController& operator=(const OscController&) = delete;
    void initialize();
    void shutdown();
    void initializeOsc(const std::string& ip, int port);
    void enableOsc(bool enable);
    bool isOscEnabled() const { return oscEnabled_; }
    void setDeviceHand(uint32_t deviceId, const std::string& hand);
    std::string getDeviceHand(uint32_t deviceId) const;
    void clearDeviceHandAssignment(uint32_t deviceId);
    void setUIManager(MainAppWindow* uiMgr);
    void setEnableOscCallback(EnableOscCallback callback);
    void handleDeviceHandAssigned(const DeviceHandAssignedEvent& event);
    void sendOscData(const FrameData& data);
    void sendHandData(const HandData& hand, const std::string& prefix);
    void sendOscBatch(const std::map<std::string, float>& dataMap);
    std::string getDevicePrefix(uint32_t deviceId);
    bool getSendPalmFlag();
    bool getSendWristFlag();
    bool getSendThumbFlag();
    bool getSendIndexFlag();
    bool getSendMiddleFlag();
    bool getSendRingFlag();
    bool getSendPinkyFlag();
    bool getSendAnyFingerFlag();
public:
    // Set the input queue for OscMessages
    void setInputQueue(ThreadSafeQueue<OscMessage>* queue) { inputQueue_ = queue; }
    // Start the processing loop in a separate thread
    void start() {
        running_ = true;
        worker_ = std::thread([this]{ run(); });
    }
    // Stop the processing loop and join the thread
    void stop() {
        running_ = false;
        if (inputQueue_) inputQueue_->notify_all();
        if (worker_.joinable()) worker_.join();
    }
    // Main processing loop: pops OscMessage and sends it
    void run() {
        while (running_) {
            if (!inputQueue_) continue;
            OscMessage msg = inputQueue_->pop();
            sendOscMessage(msg);
        }
    }
    // Send a single OscMessage using oscSender_
    void sendOscMessage(const OscMessage& msg) {
        // Logging using AppLogger
        if (logger_) { // Just check if logger exists
            logger_->log("OscController sending OSC. Enabled: " + std::string(oscEnabled_.load() ? "true" : "false") + ", Address: " + msg.address);
        }
        
        if (!oscEnabled_.load()) return; // Check if enabled
        
        // CALL the interface method directly
        oscSender_.sendOscMessage(msg); 
    }
private:
    ITransportSink& oscSender_;
    ConfigManagerInterface& configManager_;
    MainAppWindow* uiManager_;
    std::shared_ptr<AppLogger> logger_; // Added logger member
    std::atomic<bool> oscEnabled_{true};
    EnableOscCallback enableOscCallback_;
    std::mutex deviceHandMutex;
    std::map<uint32_t, std::string> deviceHandMap;
    std::atomic<bool> running_{false};
    ThreadSafeQueue<OscMessage>* inputQueue_ = nullptr;
    std::thread worker_;
};
