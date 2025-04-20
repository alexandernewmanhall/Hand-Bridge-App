#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector> // Include for std::vector
#include <cstring> // Include for strncpy
#include "../pipeline/02_LeapSorter.hpp"
#include "../core/ConfigManager.h" // Revert to using concrete class if necessary
// Or keep interface if it still makes sense: #include "../core/ConfigManagerInterface.h"
#include <memory> // Needed for shared_ptr

// Forward declare AppLogger
class AppLogger;

class UIController {
public:
    struct HandAssignmentEvent {
        std::string serialNumber;
        std::string handType;
    };
    // ...

public:
    // Define callback types for clarity (Reverted)
    using HandAssignmentCommand = std::function<void(const std::string& serial, const std::string& hand)>;
    // Reverted ConfigUpdateCommand signature (12 bools) -> Updated to 14 bools
    using ConfigUpdateCommand = std::function<void(
        bool /*palm*/, bool /*wrist*/, 
        bool /*thumb*/, bool /*index*/, bool /*middle*/, bool /*ring*/, bool /*pinky*/,
        bool /*palmOrientation*/, bool /*palmVelocity*/, bool /*palmNormal*/,
        bool /*visibleTime*/, bool /*fingerIsExtended*/,
        bool /*pinchStrength*/, bool /*grabStrength*/ // Added pinch/grab
    )>;
    using OscSettingsUpdateCallback = std::function<void(const std::string& /*newIp*/, int /*newPort*/)>;

    // Constructor - Updated to accept shared_ptr<AppLogger>
    UIController(LeapSorter& leapSorter, IConfigStore& configStore, std::shared_ptr<AppLogger> logger);

    // Set callbacks
    void setHandAssignmentCallback(HandAssignmentCommand callback);
    void setConfigUpdateCallback(ConfigUpdateCommand callback); 
    void setOscSettingsUpdateCallback(OscSettingsUpdateCallback callback);

    // Hand assignment methods
    void setDeviceHandAssignment(const std::string& serial, const std::string& hand);
    // Accessor for event queue (for processing in AppCore)
    std::vector<HandAssignmentEvent>& getHandAssignmentQueue() { return handAssignmentQueue_; }
    std::mutex& getEventQueueMutex() { return eventQueueMutex_; }

    // Getters for filter states (used by MainAppWindow) - Reverted
    bool isPalmFilterEnabled() const;
    bool isWristFilterEnabled() const;
    bool isThumbFilterEnabled() const;
    bool isIndexFilterEnabled() const;
    bool isMiddleFilterEnabled() const;
    bool isRingFilterEnabled() const;
    bool isPinkyFilterEnabled() const;
    bool isPalmOrientationFilterEnabled() const;
    bool isPalmVelocityFilterEnabled() const;
    bool isPalmNormalFilterEnabled() const;
    bool isVisibleTimeFilterEnabled() const;
    bool isFingerIsExtendedFilterEnabled() const;
    // Added getters for pinch/grab
    bool isPinchStrengthFilterEnabled() const;
    bool isGrabStrengthFilterEnabled() const;

    // Method to update individual filter settings (called by MainAppWindow)
    void setFilterState(const std::string& filterName, bool enabled);

    // Method to initialize filters from ConfigManager
    void initializeAllFilters();

    // OSC Destination Settings methods
    void initializeOscSettings(const std::string& initialIp, int initialPort);
    void applyOscSettings(); 
    char* getOscIpBuffer();
    const char* getOscIpBuffer() const;
    size_t getOscIpBufferSize() const { return sizeof(oscIpBuffer_); }
    int* getOscPortPtr();
    const int* getOscPortPtr() const;

    // REMOVED Session Timer methods
    // REMOVED Filter Change notification methods

    // MADE OSC_IP_BUFFER_SIZE public
    static constexpr size_t OSC_IP_BUFFER_SIZE = 64;

private:
    std::mutex eventQueueMutex_;
    std::vector<HandAssignmentEvent> handAssignmentQueue_;

    // Callbacks
    HandAssignmentCommand handAssignmentCommand_;
    ConfigUpdateCommand configUpdateCommand_;
    OscSettingsUpdateCallback onOscSettingsUpdate_;
    // REMOVED resetSessionTimerCallback_
    // REMOVED filterSettingsChangedCallback_

    // Internal state for OSC filters (Reverted)
    bool sendPalm_ = true;
    bool sendWrist_ = true;
    bool sendThumb_ = true;
    bool sendIndex_ = true;
    bool sendMiddle_ = true;
    bool sendRing_ = true;
    bool sendPinky_ = true;
    bool sendPalmOrientation_ = false; // Added back old filters
    bool sendPalmVelocity_ = false;
    bool sendPalmNormal_ = false;
    bool sendVisibleTime_ = false;
    bool sendFingerIsExtended_ = false;
    // REMOVED sendPinchStrength_ -> ADDED sendPinchStrength_
    // REMOVED sendGrabStrength_ -> ADDED sendGrabStrength_
    bool sendPinchStrength_ = true; // Added back, default true
    bool sendGrabStrength_ = true;  // Added back, default true

    // Internal state for OSC destination editing
    char oscIpBuffer_[OSC_IP_BUFFER_SIZE] = {0}; 
    int oscPort_ = 0; 

    // References to core components
    LeapSorter& leapSorter_;             // Reference to the sorter
    IConfigStore& configManager_;       // Now using config interface
    // Logger stored as shared_ptr now
    std::shared_ptr<AppLogger> logger_;
};
