#include "03_DataProcessor.hpp"
#include <mutex>
#include <cmath>
#include <algorithm>
#include <iostream>

// --- Normalization bounds (file scope) ---
static constexpr float X_MIN = -150.f, X_MAX = 150.f;
static constexpr float Y_MIN =  80.f,  Y_MAX = 380.f;
static constexpr float Z_MIN = -120.f, Z_MAX = 120.f;
static constexpr float X_RANGE = X_MAX - X_MIN;
static constexpr float Y_RANGE = Y_MAX - Y_MIN;
static constexpr float Z_RANGE = Z_MAX - Z_MIN;

// Corrected Constructor
DataProcessor::DataProcessor(DeviceAliasManager& aliasManager, 
                           std::function<void(const OscMessage&)> onOscMessage,
                           std::function<void(const FrameData&)> onUiEvent,
                           std::shared_ptr<AppLogger> logger) 
    : aliasManager_(aliasManager),
      onOscMessage_(std::move(onOscMessage)),
      onUiEvent_(std::move(onUiEvent)),
      logger_(std::move(logger)), 
      // Initialize ALL filters, including new ones
      sendPalm_(true), sendWrist_(true), 
      sendThumb_(true), sendIndex_(true), sendMiddle_(true), sendRing_(true), sendPinky_(true),
      sendPalmOrientation_(false), sendPalmVelocity_(false), sendPalmNormal_(false),
      sendVisibleTime_(false), sendFingerIsExtended_(false),
      sendPinchStrength_(true), sendGrabStrength_(true) 
{
    // Constructor body (if any)
}

// Helper function for sending OSC messages
void DataProcessor::sendOscMessage(const std::string& alias, const std::string& handType, const std::string& dataType, const std::string& subType, float value) {
    std::string address = "/leap/" + alias + "/" + handType + "/" + dataType;
    if (!subType.empty()) {
        address += "/" + subType;
    }
    OscMessage msg;
    msg.address = address;
    msg.values.push_back(value);
    onOscMessage_(msg);
}

// Helper function to send zero values for a specific hand type
void DataProcessor::sendZeroValues(const std::string& serialNumber, const std::string& alias, const std::string& handType) {
    // Palm
    if (sendPalm_) {
        sendOscMessage(alias, handType, "palm", "tx", 0.f);
        sendOscMessage(alias, handType, "palm", "ty", 0.f);
        sendOscMessage(alias, handType, "palm", "tz", 0.f);
    }
    // Wrist
    if (sendWrist_) {
        sendOscMessage(alias, handType, "wrist", "tx", 0.f);
        sendOscMessage(alias, handType, "wrist", "ty", 0.f);
        sendOscMessage(alias, handType, "wrist", "tz", 0.f);
    }
    // Fingers
    static const char* fingerNames[5] = {"thumb", "index", "middle", "ring", "pinky"};
    const bool fingerFilters[] = { sendThumb_, sendIndex_, sendMiddle_, sendRing_, sendPinky_ };
    for (size_t f = 0; f < 5; ++f) {
        const std::string fingerName = fingerNames[f];
        if (fingerFilters[f]) {
            sendOscMessage(alias, handType, "finger/" + fingerName, "tx", 0.f);
            sendOscMessage(alias, handType, "finger/" + fingerName, "ty", 0.f);
            sendOscMessage(alias, handType, "finger/" + fingerName, "tz", 0.f);
            sendOscMessage(alias, handType, "finger/" + fingerName, "exists", 0.f);
            if (sendFingerIsExtended_) {
                sendOscMessage(alias, handType, "finger/" + fingerName, "isExtended", 0.f);
            }
        }
    }
    // Pinch/Grab/VisibleTime
    if (sendPinchStrength_) sendOscMessage(alias, handType, "pinchStrength", "", 0.f);
    if (sendGrabStrength_) sendOscMessage(alias, handType, "grabStrength", "", 0.f);
    if (sendVisibleTime_) sendOscMessage(alias, handType, "visibleTime", "", 0.f);
}

// Updated setFilterSettings implementation (14 bools):
void DataProcessor::setFilterSettings(
    bool sendPalm, bool sendWrist,
    bool sendThumb, bool sendIndex, bool sendMiddle,
    bool sendRing, bool sendPinky,
    bool sendPalmOrientation, bool sendPalmVelocity, bool sendPalmNormal,
    bool sendVisibleTime, bool sendFingerIsExtended,
    bool sendPinchStrength, bool sendGrabStrength 
) {
    sendPalm_ = sendPalm;
    sendWrist_ = sendWrist;
    sendThumb_ = sendThumb;
    sendIndex_ = sendIndex;
    sendMiddle_ = sendMiddle;
    sendRing_ = sendRing;
    sendPinky_ = sendPinky;
    sendPalmOrientation_ = sendPalmOrientation; 
    sendPalmVelocity_ = sendPalmVelocity;
    sendPalmNormal_ = sendPalmNormal;
    sendVisibleTime_ = sendVisibleTime;
    sendFingerIsExtended_ = sendFingerIsExtended;
    sendPinchStrength_ = sendPinchStrength; 
    sendGrabStrength_ = sendGrabStrength;
}

void DataProcessor::processData(const std::string& serialNumber, const FrameData& frame) {
    std::string alias = aliasManager_.getOrAssignAlias(serialNumber);
    AssignedHand mode = aliasManager_.getAssignedHand(alias);
    auto want = [&](const std::string& ht) {
        return (mode == AssignedHand::Both) ||
               (mode == AssignedHand::Left  && ht == "left") ||
               (mode == AssignedHand::Right && ht == "right");
    };

    // Collect current hands of interest
    std::set<std::string> current;
    for (const auto& hand : frame.hands) {
        if (want(hand.handType))
            current.insert(hand.handType);
    }
    std::set<std::string>& prev = lastSeenHandsPerDevice_[alias];
    if (prev.count("left") && !current.count("left"))  sendZeroValues(serialNumber, alias, "left");
    if (prev.count("right") && !current.count("right")) sendZeroValues(serialNumber, alias, "right");
    prev.swap(current); // store for next frame

    // Normal hand processing (only for assigned hands)
    for (const auto& hand : frame.hands) {
        if (!want(hand.handType)) continue;
        // --- Raw millimetres for OSC ---
        Vector3 palmMm  = hand.palm.position;
        Vector3 wristMm = hand.arm.isValid() ? hand.arm.wristPosition : palmMm;
        std::string currentHandType = hand.handType;
        if (sendPalm_) {
            sendOscMessage(alias, currentHandType, "palm", "tx", palmMm.x);
            sendOscMessage(alias, currentHandType, "palm", "ty", palmMm.y);
            sendOscMessage(alias, currentHandType, "palm", "tz", palmMm.z);
        }
        if (sendWrist_ && hand.arm.isValid()) {
            sendOscMessage(alias, currentHandType, "wrist", "tx", wristMm.x);
            sendOscMessage(alias, currentHandType, "wrist", "ty", wristMm.y);
            sendOscMessage(alias, currentHandType, "wrist", "tz", wristMm.z);
        }
        if (sendPinchStrength_) {
            sendOscMessage(alias, currentHandType, "pinchStrength", "", hand.pinchStrength);
        }
        if (sendGrabStrength_) {
            sendOscMessage(alias, currentHandType, "grabStrength", "", hand.grabStrength);
        }
        static const char* fingerNames[5] = {"thumb", "index", "middle", "ring", "pinky"};
        const bool fingerFilters[] = { sendThumb_, sendIndex_, sendMiddle_, sendRing_, sendPinky_ };
        for (size_t f = 0; f < 5; ++f) {
            const std::string fingerName = fingerNames[f];
            const bool fingerPosEnabled = fingerFilters[f];
            const bool validFinger = f < hand.fingers.size() && hand.fingers[f].isValid() && hand.fingers[f].bones.size() > 3 && hand.fingers[f].bones[3].isValid();
            if (fingerPosEnabled && validFinger) {
                const Vector3 tipMm = hand.fingers[f].bones[3].nextJoint;
                sendOscMessage(alias, currentHandType, "finger/" + fingerName, "tx", tipMm.x);
                sendOscMessage(alias, currentHandType, "finger/" + fingerName, "ty", tipMm.y);
                sendOscMessage(alias, currentHandType, "finger/" + fingerName, "tz", tipMm.z);
            }
            if (sendFingerIsExtended_ && validFinger) {
                sendOscMessage(alias, currentHandType, "finger/" + fingerName, "isExtended", hand.fingers[f].isExtended ? 1.f : 0.f);
            }
        }
        if (sendPalmOrientation_) {
            sendOscMessage(alias, currentHandType, "palm", "orientation/qw", hand.palm.orientation.w);
            sendOscMessage(alias, currentHandType, "palm", "orientation/qx", hand.palm.orientation.x);
            sendOscMessage(alias, currentHandType, "palm", "orientation/qy", hand.palm.orientation.y);
            sendOscMessage(alias, currentHandType, "palm", "orientation/qz", hand.palm.orientation.z);
        }
        if (sendPalmVelocity_) {
            sendOscMessage(alias, currentHandType, "palm", "velocity/vx", hand.palm.velocity.x);
            sendOscMessage(alias, currentHandType, "palm", "velocity/vy", hand.palm.velocity.y);
            sendOscMessage(alias, currentHandType, "palm", "velocity/vz", hand.palm.velocity.z);
        }
        if (sendPalmNormal_) {
            sendOscMessage(alias, currentHandType, "palm", "normal/nx", hand.palm.normal.x);
            sendOscMessage(alias, currentHandType, "palm", "normal/ny", hand.palm.normal.y);
            sendOscMessage(alias, currentHandType, "palm", "normal/nz", hand.palm.normal.z);
        }
        if (sendVisibleTime_) {
            float visibleSec = static_cast<float>(hand.visibleTime) / 1'000'000.0f;
            sendOscMessage(alias, currentHandType, "visibleTime", "", visibleSec);
        }
    }
    onUiEvent_(frame);
}
