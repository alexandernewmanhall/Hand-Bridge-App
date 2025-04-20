#include "UIController.hpp"
#include "../core/AppLogger.hpp" // Include the new logger header
#include <fstream>
#include <iostream> // For logging/debugging if needed
#include "../core/Log.hpp" // Corrected logger include
#include "../core/interfaces/IConfigStore.hpp" // Include interface
#include <cstring>  // For strncpy
#include <functional>
#include <memory> // Include for shared_ptr

UIController::UIController(LeapSorter& leapSorter, IConfigStore& configStore, std::shared_ptr<AppLogger> logger)
    : leapSorter_(leapSorter),
      configManager_(configStore),
      logger_(std::move(logger)),
      sendPalm_(true), sendWrist_(true), 
      sendThumb_(true), sendIndex_(true), sendMiddle_(true), sendRing_(true), sendPinky_(true),
      sendPalmOrientation_(false), sendPalmVelocity_(false), sendPalmNormal_(false), 
      sendVisibleTime_(false), sendFingerIsExtended_(false),
      sendPinchStrength_(true), sendGrabStrength_(true)
{
    if (!logger_) {
        OutputDebugStringA("ERROR: UIController created with null logger!\n");
    }
    if (logger_) {
        logger_->log("UIController created.");
    }
}

// --- Set Callbacks --- 
void UIController::setHandAssignmentCallback(HandAssignmentCommand callback) {
    handAssignmentCommand_ = callback;
}

void UIController::setConfigUpdateCallback(ConfigUpdateCommand callback) {
    configUpdateCommand_ = callback;
}

void UIController::setOscSettingsUpdateCallback(OscSettingsUpdateCallback callback) {
    if (logger_) logger_->log("UIController: OSC settings update callback set.");
    onOscSettingsUpdate_ = callback;
}

// --- Hand Assignment --- 
void UIController::setDeviceHandAssignment(const std::string& serial, const std::string& hand) {
    if (logger_) logger_->log("UIController: Setting hand assignment for device " + serial + " to " + hand);

    // 1. Tell the Leap Sorter (Corrected method name)
    leapSorter_.setDeviceHand(serial, hand); 

    // 2. Raise the high-level callback so the UI/core can react
    if (handAssignmentCommand_) {
        handAssignmentCommand_(serial, hand);
    } else {
        if (logger_) logger_->log("WARN: UIController: handAssignmentCommand_ not set, cannot notify AppCore.");
    }

    // 3. Persist the choice (Corrected method name)
    configManager_.setDefaultHandAssignment(serial, hand);
}

// --- Filter Getters --- 
// Reverted getters to match header
bool UIController::isPalmFilterEnabled() const { return sendPalm_; }
bool UIController::isWristFilterEnabled() const { return sendWrist_; }
bool UIController::isThumbFilterEnabled() const { return sendThumb_; }
bool UIController::isIndexFilterEnabled() const { return sendIndex_; }
bool UIController::isMiddleFilterEnabled() const { return sendMiddle_; }
bool UIController::isRingFilterEnabled() const { return sendRing_; }
bool UIController::isPinkyFilterEnabled() const { return sendPinky_; }
bool UIController::isPalmOrientationFilterEnabled() const { return sendPalmOrientation_; }
bool UIController::isPalmVelocityFilterEnabled() const { return sendPalmVelocity_; }
bool UIController::isPalmNormalFilterEnabled() const { return sendPalmNormal_; }
bool UIController::isVisibleTimeFilterEnabled() const { return sendVisibleTime_; }
bool UIController::isFingerIsExtendedFilterEnabled() const { return sendFingerIsExtended_; }

// Added getters for pinch/grab
bool UIController::isPinchStrengthFilterEnabled() const {
    return sendPinchStrength_;
}

bool UIController::isGrabStrengthFilterEnabled() const {
    return sendGrabStrength_;
}

// --- Filter Initialization --- 
void UIController::initializeAllFilters() {
    if (logger_) logger_->log("UIController: Initializing all filter states from ConfigManager...");
    // Load state from ConfigManager using specific getters
    sendPalm_ = configManager_.isSendPalmEnabled();
    sendWrist_ = configManager_.isSendWristEnabled();
    sendThumb_ = configManager_.isSendThumbEnabled();
    sendIndex_ = configManager_.isSendIndexEnabled();
    sendMiddle_ = configManager_.isSendMiddleEnabled();
    sendRing_ = configManager_.isSendRingEnabled();
    sendPinky_ = configManager_.isSendPinkyEnabled();
    sendPalmOrientation_ = configManager_.isSendPalmOrientationEnabled();
    sendPalmVelocity_ = configManager_.isSendPalmVelocityEnabled();
    sendPalmNormal_ = configManager_.isSendPalmNormalEnabled();
    sendVisibleTime_ = configManager_.isSendVisibleTimeEnabled();
    sendFingerIsExtended_ = configManager_.isSendFingerIsExtendedEnabled();
    // Load pinch/grab states using new specific getters
    sendPinchStrength_ = configManager_.isSendPinchStrengthEnabled();
    sendGrabStrength_ = configManager_.isSendGrabStrengthEnabled();

    if (logger_) logger_->log("UIController: Filter states initialized. Triggering initial update to AppCore...");
    // Trigger the callback immediately to ensure DataProcessor gets the initial state
    if (configUpdateCommand_) {
         configUpdateCommand_(
            sendPalm_, sendWrist_,
            sendThumb_, sendIndex_, sendMiddle_, sendRing_, sendPinky_,
            sendPalmOrientation_, sendPalmVelocity_, sendPalmNormal_,
            sendVisibleTime_, sendFingerIsExtended_,
            sendPinchStrength_, sendGrabStrength_ // Pass pinch/grab state
        );
         if (logger_) logger_->log("UIController: Initial filter state sent to AppCore via configUpdateCommand_.");
    } else {
         if (logger_) logger_->log("WARN: UIController: Initializing filters but configUpdateCommand_ is not set!");
    }
}

// --- Filter State Update --- 
void UIController::setFilterState(const std::string& filterName, bool enabled)
{
    bool changed = false;
    // Update internal state AND call specific ConfigManager setter
    if (filterName == "sendPalm") { if(sendPalm_ != enabled) { sendPalm_ = enabled; configManager_.setSendPalmEnabled(enabled); changed = true; } }
    else if (filterName == "sendWrist") { if(sendWrist_ != enabled) { sendWrist_ = enabled; configManager_.setSendWristEnabled(enabled); changed = true; } }
    else if (filterName == "sendThumb") { if(sendThumb_ != enabled) { sendThumb_ = enabled; configManager_.setSendThumbEnabled(enabled); changed = true; } }
    else if (filterName == "sendIndex") { if(sendIndex_ != enabled) { sendIndex_ = enabled; configManager_.setSendIndexEnabled(enabled); changed = true; } }
    else if (filterName == "sendMiddle") { if(sendMiddle_ != enabled) { sendMiddle_ = enabled; configManager_.setSendMiddleEnabled(enabled); changed = true; } }
    else if (filterName == "sendRing") { if(sendRing_ != enabled) { sendRing_ = enabled; configManager_.setSendRingEnabled(enabled); changed = true; } }
    else if (filterName == "sendPinky") { if(sendPinky_ != enabled) { sendPinky_ = enabled; configManager_.setSendPinkyEnabled(enabled); changed = true; } }
    else if (filterName == "sendFingerIsExtended") { if(sendFingerIsExtended_ != enabled) { sendFingerIsExtended_ = enabled; configManager_.setSendFingerIsExtendedEnabled(enabled); changed = true; } }
    else if (filterName == "sendPalmOrientation") { if(sendPalmOrientation_ != enabled) { sendPalmOrientation_ = enabled; configManager_.setSendPalmOrientationEnabled(enabled); changed = true; } }
    else if (filterName == "sendPalmVelocity") { if(sendPalmVelocity_ != enabled) { sendPalmVelocity_ = enabled; configManager_.setSendPalmVelocityEnabled(enabled); changed = true; } }
    else if (filterName == "sendPalmNormal") { if(sendPalmNormal_ != enabled) { sendPalmNormal_ = enabled; configManager_.setSendPalmNormalEnabled(enabled); changed = true; } }
    else if (filterName == "sendVisibleTime") { if(sendVisibleTime_ != enabled) { sendVisibleTime_ = enabled; configManager_.setSendVisibleTimeEnabled(enabled); changed = true; } }
    else if (filterName == "sendPinchStrength") { if(sendPinchStrength_ != enabled) { sendPinchStrength_ = enabled; configManager_.setSendPinchStrengthEnabled(enabled); changed = true; } }
    else if (filterName == "sendGrabStrength") { if(sendGrabStrength_ != enabled) { sendGrabStrength_ = enabled; configManager_.setSendGrabStrengthEnabled(enabled); changed = true; } }
    else {
        if (logger_) logger_->log("WARN: UIController::setFilterState called with unknown filter name: " + filterName);
        return; // Unknown filter
    }

    if (changed) {
        if (logger_) logger_->log("UIController: Filter '" + filterName + "' changed to " + (enabled ? "enabled" : "disabled"));
        // Persist the change - REMOVED configManager_.setBool(...) as it's done above now
        // logger_("UIController: Persisted filter setting for '" + filterName + "'");

        // Notify AppCore via callback with the complete, current filter state (14 params)
        if (configUpdateCommand_) {
            configUpdateCommand_(
                sendPalm_, sendWrist_,
                sendThumb_, sendIndex_, sendMiddle_, sendRing_, sendPinky_,
                sendPalmOrientation_, sendPalmVelocity_, sendPalmNormal_,
                sendVisibleTime_, sendFingerIsExtended_,
                sendPinchStrength_, sendGrabStrength_ // Pass pinch/grab state
            );
            if (logger_) logger_->log("UIController: Notified AppCore via configUpdateCommand_ with all 14 filter states.");
        } else {
            if (logger_) logger_->log("WARN: UIController: Filter state changed but configUpdateCommand_ is not set!");
        }
    }
}

// --- OSC Settings --- 
void UIController::initializeOscSettings(const std::string& initialIp, int initialPort) {
    if (logger_) logger_->log("UIController: Initializing OSC settings: IP=" + initialIp + ", Port=" + std::to_string(initialPort));
    strncpy_s(oscIpBuffer_, OSC_IP_BUFFER_SIZE, initialIp.c_str(), _TRUNCATE);
    oscPort_ = initialPort;
}

void UIController::applyOscSettings() {
    if (onOscSettingsUpdate_) {
        std::string currentIp(oscIpBuffer_);
        if (logger_) logger_->log("UIController: Applying OSC settings: IP=" + currentIp + ", Port=" + std::to_string(oscPort_));
        onOscSettingsUpdate_(currentIp, oscPort_);
    } else {
        if (logger_) logger_->log("WARN: UIController: Apply OSC settings called, but no update callback is set.");
    }
}

// --- Compatibility methods --- 
char* UIController::getOscIpBuffer() { return oscIpBuffer_; }
const char* UIController::getOscIpBuffer() const { return oscIpBuffer_; }
int* UIController::getOscPortPtr() { return &oscPort_; }
const int* UIController::getOscPortPtr() const { return &oscPort_; }
