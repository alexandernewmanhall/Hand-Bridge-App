#pragma once
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include "../core/FrameData.hpp"
#include "transport/osc/OscMessage.hpp"
#include "../core/DeviceAliasManager.hpp"
#include "../core/AppLogger.hpp"

class DataProcessor {
public:
    // Callback types
    using OscMessageCallback = std::function<void(const OscMessage&)>;
    using UiEventCallback = std::function<void(const FrameData&)>;

    /**
     * @param aliasManager Reference to DeviceAliasManager for serial-to-alias mapping.
     * @param onOscMessage Callback for OSC message emission.
     * @param onUiEvent Callback for UI event emission.
     * @param logger Shared pointer to the application logger.
     */
    DataProcessor(DeviceAliasManager& aliasManager, OscMessageCallback onOscMessage, UiEventCallback onUiEvent, std::shared_ptr<AppLogger> logger);

    // processData signature
    void processData(const std::string& serialNumber, const FrameData& frame);
    
    // setFilterSettings declaration
    void setFilterSettings(bool sendPalm, bool sendWrist, 
                           bool sendThumb, bool sendIndex, bool sendMiddle, bool sendRing, bool sendPinky,
                           bool sendPalmOrientation, bool sendPalmVelocity, bool sendPalmNormal,
                           bool sendVisibleTime, bool sendFingerIsExtended,
                           bool sendPinchStrength, bool sendGrabStrength);

private:
    // Helper function to send zero values for a specific hand type
    void sendZeroValues(const std::string& serialNumber, const std::string& alias, const std::string& handType);
    // Helper function for sending OSC messages
    void sendOscMessage(const std::string& alias, const std::string& handType, const std::string& dataType, const std::string& subType, float value);

    DeviceAliasManager& aliasManager_;
    OscMessageCallback onOscMessage_;
    UiEventCallback onUiEvent_;
    std::shared_ptr<AppLogger> logger_; 

    // Reverted filter states
    std::atomic<bool> sendPalm_{true};
    std::atomic<bool> sendWrist_{true};
    std::atomic<bool> sendThumb_{true};
    std::atomic<bool> sendIndex_{true};
    std::atomic<bool> sendMiddle_{true};
    std::atomic<bool> sendRing_{true};
    std::atomic<bool> sendPinky_{true};
    std::atomic<bool> sendPalmOrientation_{false};
    std::atomic<bool> sendPalmVelocity_{false};
    std::atomic<bool> sendPalmNormal_{false};
    std::atomic<bool> sendVisibleTime_{false};
    std::atomic<bool> sendFingerIsExtended_{false};
    // Added pinch/grab filters
    std::atomic<bool> sendPinchStrength_{true};
    std::atomic<bool> sendGrabStrength_{true};  

    // State tracking for zeroing
    std::map<std::string, std::set<std::string>> lastSeenHandsPerDevice_;
    std::mutex lastSeenHandsMutex_;

    // Per-hand state for velocity/gain (by handType: "left"/"right")
    struct HandMotionState {
        // Palm
        Vector3 prevPos_mm; 
        uint64_t prevTimestamp = 0;
        Vector3 filteredVelocity = {0,0,0}; 
        Vector3 cursorNorm = {0.5f, 0.5f, 0.5f}; 
        Vector3 prevCursorNorm = {0.5f, 0.5f, 0.5f}; 

        // Wrist
        Vector3 prevWristPos_mm; 
        Vector3 wristCursorNorm = {0.5f, 0.5f, 0.5f}; 
        Vector3 prevWristCursorNorm = {0.5f, 0.5f, 0.5f}; 

        // Finger Tips (Index 0=Thumb, 4=Pinky)
        Vector3 prevTipPos_mm[5]; 
        Vector3 tipCursorNorm[5] = {{0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}}; 
        Vector3 prevTipCursorNorm[5] = {{0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}}; 
    };
    std::map<std::string, HandMotionState> handMotionStates_; 

};
