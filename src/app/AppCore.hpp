#pragma once

// Include full definitions for owned members
#include "../pipeline/00_LeapConnection.hpp"
#include "../pipeline/01_LeapPoller.hpp"
#include "../core/IFrameStreamingInputDevice.hpp"
#include "../core/interfaces/IFrameSource.hpp"
#include <memory>
#include "../pipeline/02_LeapSorter.hpp"
#include "../pipeline/03_DataProcessor.hpp"
#include "../pipeline/04_OscSender.hpp"
#include "../core/interfaces/ITransportSink.hpp" // Correct path for interface
#include "../ui/UIController.hpp"
#include "../core/DeviceAliasManager.hpp"

// Forward declare dependencies passed by reference
#include "../core/interfaces/IConfigStore.hpp"
#include "core/FrameData.hpp" // Include FrameData for queue
#include "utils/SpscQueue.hpp" // Include SpscQueue
#include <memory> // Ensure shared_ptr is available
class MainAppWindow;
struct FrameData; // Can likely remain forward-declared
class AppLogger; // Add forward declaration for AppLogger

// Include standard libraries needed
#include <functional>
#include <string>
#include <iostream> // For default logger lambda
#include <atomic>
#include "transport/osc/OscController.h" // Make sure this is included

class AppCore {
public:
    // Updated Constructor Signature (Remove queue param)
    AppCore(std::shared_ptr<IConfigStore> configManager,
            MainAppWindow& uiManager, // Keep as reference if ownership is external
            std::shared_ptr<AppLogger> logger);
    ~AppCore();

    // No copying/moving
    AppCore(const AppCore&) = delete;
    AppCore& operator=(const AppCore&) = delete;

    void start();
    void stop();
    bool isRunning() const { return isRunning_; }
    // For test/demo: emit a test frame
    void emitTestFrame(const std::string& deviceId, const FrameData& frame);
    // Add consumer method
    int processPendingFrames();
    void processQueuedHandAssignments();

    // --- DataProcessor accessor for startup configuration ---
    DataProcessor* getDataProcessor() { return dataProcessor_.get(); }
    OscController* getOscController(); // <-- ADD THIS DECLARATION

private:
    // Event Handlers (implement in .cpp)
    void handleDeviceConnected(const LeapPoller::DeviceInfo& info); // Uses definition from 01_LeapPoller.hpp
    void handleDeviceLost(const std::string& serialNumber);

    // Core Components (Initialize in constructor)
    LeapConnection connectionManager_;
    std::unique_ptr<IFrameStreamingInputDevice> leapInput_;
    // Abstract interface for frame source (non-owning, points to leapInput_)
    IFrameSource* frameSource_ = nullptr;
    LeapSorter leapSorter_;
    // DataProcessor is now a unique_ptr
    std::unique_ptr<DataProcessor> dataProcessor_;
    std::unique_ptr<ITransportSink> oscSender_; // Use interface for transport sink

    // Queue for decoupling polling thread from main thread (SHARED OWNERSHIP)
    std::shared_ptr<SpscQueue<FrameData>> frameDataQueue_;

    // References to external/UI/Config components (passed in constructor)
    std::shared_ptr<IConfigStore> configManager_; // Use config interface
    MainAppWindow& uiManager_;           // Use reference to MainAppWindow
    std::unique_ptr<UIController> uiController_; // Changed to unique_ptr

    // Utilities
    std::shared_ptr<AppLogger> logger_; // Logger function object
    std::atomic<bool> isRunning_ = false;

    // Assume oscController_ is the member holding the instance
    std::unique_ptr<OscController> oscController_; 
    // Or maybe OscSenderStage oscSenderStage_ and OscController depends on it?
    // Check AppCore.cpp for actual members.
};
