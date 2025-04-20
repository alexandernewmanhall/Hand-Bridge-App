#pragma once

#include <SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <optional>
#include <mutex>
#include <map>
// Event type forward declarations (removed event/EventTypes.h)
struct TrackingDataEvent;
struct ConnectEvent;
struct DisconnectEvent;
struct DeviceConnectedEvent;
struct DeviceLostEvent;
struct DeviceHandAssignedEvent;
#include "OpenGLRenderer.h"

// Forward declarations
class OscController;
#include "../core/ConfigManagerInterface.h"
// class DeviceManager; // Removed for new pipeline


#include "../../third_party/json.hpp"

class UIController; // Forward declaration

class MainAppWindow {
public:
    // --- OSC filter flags ---
    bool sendPalm = true;
    bool sendWrist = true;
    bool sendThumb = true;
    bool sendIndex = true;
    bool sendMiddle = true;
    bool sendRing = true;
    bool sendPinky = true;

    MainAppWindow(std::function<void(const TrackingDataEvent&)> onTrackingData,
             std::function<void(const ConnectEvent&)> onConnect,
             std::function<void(const DisconnectEvent&)> onDisconnect,
             std::function<void(const DeviceConnectedEvent&)> onDeviceConnected,
             std::function<void(const DeviceLostEvent&)> onDeviceLost,
             std::function<void(const DeviceHandAssignedEvent&)> onDeviceHandAssigned);

    ~MainAppWindow();

    // Initialization and shutdown
    bool init(const char* title, int width, int height);
    void shutdown();

    // Settings persistence
    void saveSettings();
    void loadSettings();
    
    // Main loop methods
    void processData(SDL_Event* event);
    void processEvent(SDL_Event* event);
    void render();
    void resize(int width, int height);
    
    // Window access
    SDL_Window* getWindow() { return window; }
    
    // Set the controllers/managers to interact with
    void setControllers(ConfigManagerInterface* configManager, OscController* oscController);
    void setUIController(UIController* uiController);

    // --- OSC Data Filtering State --- 
    bool sendFingers = true; // Grouped toggle for all fingers
    // Add more flags here if needed for other data types (e.g., confidence, grab/pinch)
    // ---------------------------------

    // Data selection getter methods for OscController
    bool isPalmDataEnabled() const { return sendPalm; }
    bool isWristDataEnabled() const { return sendWrist; }
    bool isThumbDataEnabled() const { return sendThumb; }
    bool isIndexDataEnabled() const { return sendIndex; }
    bool isMiddleDataEnabled() const { return sendMiddle; }
    bool isRingDataEnabled() const { return sendRing; }
    bool isPinkyDataEnabled() const { return sendPinky; }

    // UI rendering helper function
    void renderMainUI();

    // Event handlers for tracking data display
    void handleTrackingData(const TrackingDataEvent& event);
    void handleConnect(const ConnectEvent& event);
    void handleDisconnect(const DisconnectEvent& event);
    void handleDeviceConnected(const DeviceConnectedEvent& event);
    void handleDeviceLost(const DeviceLostEvent& event);
    void handleDeviceHandAssigned(const DeviceHandAssignedEvent& event);

private:
    UIController* uiController_ = nullptr;
    // Helper to get the settings file path
    std::string getSettingsFilePath() const;
    
    // Subscribe to relevant events
    void subscribeToEvents();

    // SDL window
    SDL_Window* window = nullptr;
    int windowWidth = 0;
    int windowHeight = 0;
    
    // Controllers/managers references
    ConfigManagerInterface* configManager = nullptr;
    OscController* oscController = nullptr;
    
    // DeviceManager* deviceManager = nullptr; // Removed for new pipeline

    // Tracking data for display - Changed to support multiple devices
    struct PerDeviceTrackingData {
        std::string serialNumber; // Store serial number here too
        bool isConnected = false;
        std::string assignedHand = "";
        std::atomic<double> frameRate = { 0.0 };
        std::atomic<int> handCount = { 0 };
        std::chrono::high_resolution_clock::time_point lastFrameTime;
        uint64_t frameCount = 0;
    };

    // Use std::string (serial number) as the key
    std::map<std::string, PerDeviceTrackingData> deviceTrackingDataMap;
    std::mutex trackingDataMutex;

    // Overall connection status (independent of individual devices)
    std::atomic<bool> isLeapConnected = {false};

    // Status messages (can remain general)
    std::vector<std::string> statusMessages;
    std::mutex statusMutex;
    void addStatusMessage(const std::string& message);
    std::vector<std::string> getStatusMessages();

    // Event subscriptions using std::optional for delayed initialization


    // OpenGL renderer
    OpenGLRenderer renderer;

    // Dependency-injected event callbacks
    std::function<void(const TrackingDataEvent&)> onTrackingData;
    std::function<void(const ConnectEvent&)> onConnect;
    std::function<void(const DisconnectEvent&)> onDisconnect;
    std::function<void(const DeviceConnectedEvent&)> onDeviceConnected;
    std::function<void(const DeviceLostEvent&)> onDeviceLost;
    std::function<void(const DeviceHandAssignedEvent&)> onDeviceHandAssigned;
    
    // ImGui initialization
    bool initImGui();
    void shutdownImGui();
    
    // Frame rendering
    bool recreateRenderer();
    
    // Helper functions
    static void framebufferResizeCallback(SDL_Window* window, int width, int height);

    // --- Added Helper Function --- 
    PerDeviceTrackingData& getDeviceData(const std::string& serialNumber); // Helper to get/create device data entry
}; 