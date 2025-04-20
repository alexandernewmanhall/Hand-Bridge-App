#pragma once

// --- Include Windows FIRST for HWND --- 
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
// --- End Windows Include --- 

#include <SDL.h> // Restore SDL include
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <optional>
#include <mutex>
#include <map>
#include <functional> // Make sure functional is included
#include <SDL.h>

// Event type includes - Use FrameData instead of TrackingDataEvent
// #include "../core/TrackingDataEvent.hpp" // REMOVED
#include "../core/FrameData.hpp"           // ADDED (If not already present)
#include "../core/ConnectEvent.hpp"
#include "UIController.hpp" // Needed for interactions
#include "json.hpp" // Ensure json is included here
#include "../core/Log.hpp" // Changed from .hpp to .h
#include "../core/DeviceHandAssignedEvent.hpp" // Added this include
#include "../core/DisconnectEvent.hpp"
#include "../core/DeviceConnectedEvent.hpp"
#include "../core/DeviceLostEvent.hpp"
#include "transport/osc/OscController.h"
#include "OpenGLRenderer.h"

// Forward declarations to avoid full include in the header
class DataProcessor;        // lives in 03_DataProcessor.hpp
struct AspectPreset;        // same header

// Forward declare SDL_Window instead of including SDL.h
struct SDL_Window;
struct SDL_Renderer; // If using SDL_Renderer
struct ImGuiIO;

// Custom deleter for SDL_Window
struct SdlWindowDeleter {
    void operator()(SDL_Window* window) const {
        if (window) {
            SDL_DestroyWindow(window);
        }
    }
};

// TODO: Use SdlWindowDeleter with std::unique_ptr<SDL_Window, SdlWindowDeleter> window_ for exception safety.


// Forward declare the test fixture class for the friend declaration below
class MainAppWindowTest_FRIEND;

class MainAppWindow {
    // Grant test access to private members
    friend void swap(MainAppWindow& first, MainAppWindow& second); // For copy-and-swap

    // Grant access to the test fixture for private members
    friend class MainAppWindowTest_FRIEND; // CORRECTED NAME

public:
    struct PerDeviceTrackingData {
        std::string serialNumber;
        bool isConnected = false;
        DeviceHandAssignedEvent::HandType assignedHand = DeviceHandAssignedEvent::HandType::HAND_NONE;
        std::atomic<double> frameRate = { 0.0 };
        std::atomic<int> handCount = { 0 };
        std::chrono::high_resolution_clock::time_point lastFrameTime;
        uint64_t frameCount = 0;
        std::atomic<float> leftPinchStrength = { 0.0f };
        std::atomic<float> leftGrabStrength = { 0.0f };
        std::atomic<float> rightPinchStrength = { 0.0f };
        std::atomic<float> rightGrabStrength = { 0.0f };
    };
    /**
     * Must be called before first render() to ensure dataProcessor is valid.
     * This links the UI to the DataProcessor. See also reciprocal note in DataProcessor.
     */
    void setDataProcessor(DataProcessor* dp);

    // Getters for associated controllers/managers
    ConfigManagerInterface* getConfigManager() { return configManager; }
    const ConfigManagerInterface* getConfigManager() const { return configManager; } // const version
    OscController* getOscController() { return oscController; }
    const OscController* getOscController() const { return oscController; } // const version

public:
    // == Temporarily Public for Debugging ==
    // ... OSC flags (to be removed later) ...
    // TODO: Add VSync toggle to UI for user experience.
    // TODO: Ensure shutdown order if new SDL resources are added.
    // TODO: ImGui theme customization improvements.
    // TODO: Handle exceptions safely in constructor (RAII resource acquisition).

    // --- Added declarations to match .cpp ---
    std::vector<std::string> getStatusMessages();
    PerDeviceTrackingData& getDeviceData(const std::string& serialNumber);
    const PerDeviceTrackingData& getDeviceData(const std::string& serialNumber) const;

    MainAppWindow(std::function<void(const FrameData&)> onTrackingData,
             std::function<void(const ConnectEvent&)> onConnect,
             std::function<void(const DisconnectEvent&)> onDisconnect,
             std::function<void(const DeviceConnectedEvent&)> onDeviceConnected,
             std::function<void(const DeviceLostEvent&)> onDeviceLost,
             std::function<void(const DeviceHandAssignedEvent&)> onDeviceHandAssigned,
             std::function<void(const std::string&)> logger);
    ~MainAppWindow();

    // Initialization and shutdown
    bool init(const char* title, int width, int height);
    void shutdown();

    // Settings persistence - REMOVED
    // void saveSettings();
    // void loadSettings();
    
    // Main loop methods
    void processEvent(SDL_Event* event);
    void render();
    void resize(int width, int height);
    // void processData(SDL_Event* event); // Is this needed? Removed for now.
    
    // Window access
    SDL_Window* getWindow();
    
    // Set the controllers/managers to interact with
    void setControllers(ConfigManagerInterface* configManager, OscController* oscController);
    void setUIController(UIController* uiController);
    void setAliasLookupFunction(std::function<std::string(const std::string&)> func);

    // ... OSC flags getters (to be removed later) ...

    // UI rendering helper function
    void renderMainUI();

    // Event handlers - UPDATED handleTrackingData signature
    void handleTrackingData(const FrameData& frame);
    void handleConnect(const ConnectEvent& event);
    void handleDisconnect(const DisconnectEvent& event);
    void handleDeviceConnected(const DeviceConnectedEvent& event);
    void handleDeviceLost(const DeviceLostEvent& event);
    void handleDeviceHandAssigned(const DeviceHandAssignedEvent& event);

    // Dependency-injected event callbacks - UPDATED onTrackingData type
    std::function<void(const FrameData&)> onTrackingData;
    std::function<void(const ConnectEvent&)> onConnect;
    std::function<void(const DisconnectEvent&)> onDisconnect;
    std::function<void(const DeviceConnectedEvent&)> onDeviceConnected;
    std::function<void(const DeviceLostEvent&)> onDeviceLost;
    std::function<void(const DeviceHandAssignedEvent&)> onDeviceHandAssigned;
    
    // Logger function member
    std::function<void(const std::string&)> logger_;

    // NEW: Method to get the native window handle
#if defined(_WIN32)
    HWND getHWND();
#else
    void* getHWND(); // Provide a dummy for non-Windows
#endif

private:
    // ---- Moved Member Variables First ----
    DataProcessor* dataProcessor = nullptr;
    // Use unique_ptr with custom deleter for the window
    std::unique_ptr<SDL_Window, SdlWindowDeleter> window_ = nullptr;
    bool imguiInitialized = false;
    int windowWidth = 0;
    int windowHeight = 0;
    OpenGLRenderer renderer;
    ConfigManagerInterface* configManager = nullptr;
    OscController* oscController = nullptr;
    UIController* uiController_ = nullptr;

    // ---- Define DeviceRowDisplayData AFTER ----
    struct DeviceRowDisplayData {
        std::string serial;
        std::string alias;
        const PerDeviceTrackingData* deviceData; // Now PerDeviceTrackingData is fully defined
    };
    // ---- End DeviceRowDisplayData Definition ----

    std::map<std::string, PerDeviceTrackingData> deviceTrackingDataMap;
    std::mutex trackingDataMutex;

    // Connection Status
    std::atomic<bool> isLeapConnected = {false};

    // Status Messages
    std::vector<std::string> statusMessages;
    std::mutex statusMutex;

    // Add member for alias lookup function
    std::function<std::string(const std::string&)> aliasLookupFunc_;

    // Private Methods
    void subscribeToEvents();
    void renderMenuBar();
    void renderDevicePanel();
    void renderOscSettingsPanel();
    void renderStatusMessagesPanel();
    void renderAboutPanel();
    bool initImGui();
    void shutdownImGui();
    bool recreateRenderer();

    // NEW: Store HWND after init
#if defined(_WIN32)
    HWND nativeWindowHandle_ = NULL;
#else
    void* nativeWindowHandle_ = nullptr; // Provide a dummy for non-Windows
#endif

    std::vector<std::string> getStatusMessages() const; // Moved back to private
    void addStatusMessage(const std::string& message); // Moved back to private
};