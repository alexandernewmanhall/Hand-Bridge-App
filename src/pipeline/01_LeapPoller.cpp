#include "01_LeapPoller.hpp"
#include "core/Log.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <cstring> // For strcmp
#include <windows.h>
#include "../core/FrameData.hpp" // Include FrameData for conversion
#include "../core/HandData.hpp" // Include HandData for conversion

namespace { // Put in anonymous namespace
FrameData convertLeapToFrameData(const LEAP_TRACKING_EVENT* tracking, const std::string& deviceSerial) {
    FrameData frame;
    if (!tracking) return frame;

    frame.deviceId = deviceSerial; // Assign deviceId
    frame.timestamp = tracking->info.timestamp;

    for (uint32_t i = 0; i < tracking->nHands; ++i) {
        const LEAP_HAND& srcHand = tracking->pHands[i];
        HandData hand;
        hand.handType = srcHand.type == eLeapHandType_Left ? "left" : "right";
        // ... (copy palm, arm, fingers, etc. - same as before) ...
        hand.palm.position = { srcHand.palm.position.x, srcHand.palm.position.y, srcHand.palm.position.z };
        hand.palm.velocity = { srcHand.palm.velocity.x, srcHand.palm.velocity.y, srcHand.palm.velocity.z };
        hand.palm.normal = { srcHand.palm.normal.x, srcHand.palm.normal.y, srcHand.palm.normal.z };
        hand.palm.direction = { srcHand.palm.direction.x, srcHand.palm.direction.y, srcHand.palm.direction.z };
        hand.palm.orientation = { srcHand.palm.orientation.w, srcHand.palm.orientation.x, srcHand.palm.orientation.y, srcHand.palm.orientation.z };
        hand.palm.width = srcHand.palm.width;
        // Arm
        hand.arm.wristPosition = { srcHand.arm.next_joint.x, srcHand.arm.next_joint.y, srcHand.arm.next_joint.z };
        hand.arm.elbowPosition = { srcHand.arm.prev_joint.x, srcHand.arm.prev_joint.y, srcHand.arm.prev_joint.z };
        hand.arm.width = srcHand.arm.width;
        hand.arm.rotation = { srcHand.arm.rotation.w, srcHand.arm.rotation.x, srcHand.arm.rotation.y, srcHand.arm.rotation.z };
        // Fingers
        for (int f = 0; f < 5; ++f) {
            const LEAP_DIGIT& srcFinger = srcHand.digits[f];
            FingerData finger;
            finger.fingerId = srcFinger.finger_id;
            finger.isExtended = srcFinger.is_extended != 0;
            for (int b = 0; b < 4; ++b) {
                const LEAP_BONE& srcBone = srcFinger.bones[b];
                BoneData bone;
                bone.prevJoint = { srcBone.prev_joint.x, srcBone.prev_joint.y, srcBone.prev_joint.z };
                bone.nextJoint = { srcBone.next_joint.x, srcBone.next_joint.y, srcBone.next_joint.z };
                bone.width = srcBone.width;
                bone.rotation = { srcBone.rotation.w, srcBone.rotation.x, srcBone.rotation.y, srcBone.rotation.z };
                finger.bones.push_back(bone);
            }
            hand.fingers.push_back(finger);
        }
        hand.pinchStrength = srcHand.pinch_strength;
        hand.grabStrength = srcHand.grab_strength;
        hand.confidence = srcHand.confidence;
        hand.visibleTime = srcHand.visible_time;
        frame.hands.push_back(hand);
    }
    return frame;
}
} // end anonymous namespace

LeapPoller::LeapPoller(LEAP_CONNECTION connection)
    : connection_(connection) {}

LeapPoller::~LeapPoller() {
    cleanup();
}

const char* LeapPoller::GetLeapRSString(eLeapRS code) {
    switch (code) {
    case eLeapRS_Success:               return "Success";
    case eLeapRS_UnknownError:          return "Unknown Error";
    case eLeapRS_InvalidArgument:       return "Invalid Argument";
    case eLeapRS_InsufficientResources: return "Insufficient Resources";
    case eLeapRS_InsufficientBuffer:    return "Insufficient Buffer";
    case eLeapRS_Timeout:               return "Timeout";
    case eLeapRS_NotConnected:          return "Not Connected";
    case eLeapRS_HandshakeIncomplete:   return "Handshake Incomplete";
    case eLeapRS_BufferSizeOverflow:    return "Buffer Size Overflow";
    case eLeapRS_ProtocolError:         return "Protocol Error";
    default:                            return "Unknown Code";
    }
}

bool LeapPoller::getDeviceSerial(LEAP_DEVICE device, std::string& serialNumber) {
    LEAP_DEVICE_INFO deviceInfo = { sizeof(LEAP_DEVICE_INFO) };
    eLeapRS result = LeapGetDeviceInfo(device, &deviceInfo);
    if (result != eLeapRS_Success) {
        char fallbackId[32];
        sprintf_s(fallbackId, "fallback_%p", (void*)device);
        serialNumber = fallbackId;
        return false;
    }
    deviceInfo.serial = nullptr;
    result = LeapGetDeviceInfo(device, &deviceInfo);
    if (result != eLeapRS_Success || deviceInfo.serial_length == 0) {
        char fallbackId[32];
        sprintf_s(fallbackId, "fallback_%p", (void*)device);
        serialNumber = fallbackId;
        return false;
    }
    char* buffer = new char[deviceInfo.serial_length];
    deviceInfo.serial = buffer;
    result = LeapGetDeviceInfo(device, &deviceInfo);
    if (result != eLeapRS_Success) {
        delete[] buffer;
        char fallbackId[32];
        sprintf_s(fallbackId, "fallback_%p", (void*)device);
        serialNumber = fallbackId;
        return false;
    }
    serialNumber = buffer;
    delete[] buffer;
    return true;
}

#include "../../LeapSDK/include/LeapC.h"

bool LeapPoller::initializeDevices() {
    // Enumerate and open Leap devices
    uint32_t deviceCount = 0;
    eLeapRS result = LeapGetDeviceList(connection_, nullptr, &deviceCount);
    if (result != eLeapRS_Success || deviceCount == 0) {
        LOG_ERR("No Leap devices found. (result=" << GetLeapRSString(result) << ", count=" << deviceCount << ")");
        return false;
    }
    LOG("Leap device count: " << deviceCount);
    std::vector<LEAP_DEVICE_REF> deviceRefs(deviceCount);
    result = LeapGetDeviceList(connection_, deviceRefs.data(), &deviceCount);
    if (result != eLeapRS_Success) {
        LOG_ERR("Failed to get Leap device list. (result=" << GetLeapRSString(result) << ")");
        return false;
    }
    devices_.clear();
    for (uint32_t i = 0; i < deviceCount; ++i) {
        LEAP_DEVICE deviceHandle = nullptr;
        result = LeapOpenDevice(deviceRefs[i], &deviceHandle);
        if (result == eLeapRS_Success && deviceHandle) {
            // Subscribe to events for this device (CRITICAL FOR MULTI-DEVICE)
            eLeapRS subResult = LeapSubscribeEvents(connection_, deviceHandle);
            if (subResult != eLeapRS_Success) {
                LOG_ERR("Failed to subscribe to initial device events for handle=" << static_cast<void*>(deviceHandle) << ", result=" << GetLeapRSString(subResult) << ". Tracking might not work for this device.");
                LeapCloseDevice(deviceHandle);
                continue;
            }

            DeviceInfo info;
            info.deviceHandle = deviceHandle;
            info.id = deviceRefs[i].id; // Set device id from LEAP_DEVICE_REF
            getDeviceSerial(deviceHandle, info.serialNumber);
            if (!info.serialNumber.empty() && info.serialNumber.find("fallback_") == std::string::npos) {
                devices_.push_back(info);
                LOG("Opened and Subscribed to Leap device, id: " << info.id << ", serial: " << info.serialNumber);
            } else {
                LOG_ERR("Failed to get valid serial for device handle " << static_cast<void*>(deviceHandle) << ". Skipping.");
                LeapUnsubscribeEvents(connection_, deviceHandle);
                LeapCloseDevice(deviceHandle); // Close device if we can't identify it properly
            }
        } else {
            LOG_ERR("Failed to open Leap device " << i << " (result=" << GetLeapRSString(result) << ")");
        }
    }
    return !devices_.empty();
}

void LeapPoller::handleDeviceEvent(const LEAP_DEVICE_EVENT* deviceEvent) {
    // Find the device info
    if (!deviceEvent) return;

    // Check if device is already known
    for (const auto& info : devices_) {
        if (info.id == deviceEvent->device.id) {
            // Already known, just call callback if needed (e.g., for re-connection)
            if (onDeviceConnected_) {
                onDeviceConnected_(info);
            }
            LOG("Device re-connected: id = " << info.id << ", handle = " << static_cast<void*>(deviceEvent->device.handle) << ", serial = " << info.serialNumber);
            return; // Already handled
        }
    }

    // Device not found, it's a new connection
    LOG("New device connected: handle = " << static_cast<void*>(deviceEvent->device.handle));

    // Create new device info
    DeviceInfo newInfo;
    newInfo.id = deviceEvent->device.id;
    // Attempt to open the device to get a persistent handle
    eLeapRS result = LeapOpenDevice(deviceEvent->device, &newInfo.deviceHandle);
    if (result != eLeapRS_Success || !newInfo.deviceHandle) {
        LOG_ERR("Failed to open newly connected device handle: " << static_cast<void*>(deviceEvent->device.handle) << ", error: " << GetLeapRSString(result));
        return; // Cannot proceed without a valid handle
    }

    // Subscribe to events for this device (CRITICAL FOR MULTI-DEVICE)
    eLeapRS subResult = LeapSubscribeEvents(connection_, newInfo.deviceHandle);
    if (subResult != eLeapRS_Success) {
        LOG_ERR("Failed to subscribe to device events for device id=" << newInfo.id << ", handle=" << static_cast<void*>(newInfo.deviceHandle) << ", result=" << GetLeapRSString(subResult) << ". Tracking might not work for this device.");
        // Optionally decide if you want to close the device if subscription fails,
        // depending on whether you can tolerate tracking not working for it.
        // LeapCloseDevice(newInfo.deviceHandle);
        // return;
    }

    // Get serial number
    if (!getDeviceSerial(newInfo.deviceHandle, newInfo.serialNumber) || newInfo.serialNumber.empty() || newInfo.serialNumber.find("fallback_") != std::string::npos) {
        LOG_ERR("Failed to get valid serial number for new device handle: " << static_cast<void*>(newInfo.deviceHandle) << ". Cannot add device.");
        LeapUnsubscribeEvents(connection_, newInfo.deviceHandle);
        LeapCloseDevice(newInfo.deviceHandle); // Close the device we just opened
        return; // Cannot add device without a valid serial
    }

    // Check if serial already exists (e.g., rapid reconnect)
    for (const auto& existingInfo : devices_) {
        if (existingInfo.serialNumber == newInfo.serialNumber || existingInfo.id == newInfo.id) {
            std::cerr << "[LeapPoller] Warning: Device with id " << newInfo.id << " or serial " << newInfo.serialNumber.c_str() << " already exists. Ignoring duplicate connection event." << std::endl;
            LeapUnsubscribeEvents(connection_, newInfo.deviceHandle);
            LeapCloseDevice(newInfo.deviceHandle); // Close the duplicate handle
            return;
        }
    }

    // Add to list and invoke callback
    devices_.push_back(newInfo);
    LOG("Added new device: id = " << newInfo.id << ", serial = " << newInfo.serialNumber);

    if (onDeviceConnected_) {
        onDeviceConnected_(newInfo);
    }
}

void LeapPoller::handleDeviceLost(const LEAP_DEVICE_EVENT* deviceEvent) {
    if (!deviceEvent) return;

    std::string lostSerialNumber = "";
    uint32_t lostId = 0; // Store ID temporarily
    auto it = std::find_if(devices_.begin(), devices_.end(), [deviceEvent](const DeviceInfo& info) {
        return info.id == deviceEvent->device.id;
    });

    if (it != devices_.end()) {
        lostSerialNumber = it->serialNumber;
        lostId = it->id; // Get the ID
        LOG("Device lost: id = " << lostId << ", handle = " << static_cast<void*>(deviceEvent->device.handle) << ", serial = " << lostSerialNumber);

        // Notify listeners *before* unsubscribing/closing
        if (onDeviceLost_) {
            onDeviceLost_(lostSerialNumber);
        }

        // Unsubscribe and close the device handle associated with this entry before erasing
        if(it->deviceHandle) {
            eLeapRS unsubResult = LeapUnsubscribeEvents(connection_, it->deviceHandle);
            if (unsubResult != eLeapRS_Success) {
                 LOG_ERR("Failed to unsubscribe from device id=" << lostId << ", handle=" << static_cast<void*>(it->deviceHandle) << ", result=" << GetLeapRSString(unsubResult));
            }
            LeapCloseDevice(it->deviceHandle);
        }
        devices_.erase(it);
    } else {
        std::cerr << "[LeapPoller] Warning: DeviceLost event for unknown id: " << deviceEvent->device.id << std::endl;
    }
}

// This function now converts the event and calls the callback
void LeapPoller::handleTracking(const LEAP_TRACKING_EVENT* tracking, const std::string& serialNumber) {
    LOG("LeapPoller::handleTracking called for SN: " + serialNumber);
    // Check if the callback is valid before proceeding
    if (frameCallback_) {
        // Perform conversion here
        FrameData frame = convertLeapToFrameData(tracking, serialNumber); // Use the converter
        // Call the callback with the converted data
        frameCallback_(frame);
    } else {
        // Optional: Log that frame was skipped due to null callback (might be noisy)
        // std::cerr << "[LeapPoller] Frame skipped for device " << serialNumber << ": callback is null." << std::endl;
    }
}

void LeapPoller::poll() {
    // Poll Leap connection for a single event or timeout
    LEAP_CONNECTION_MESSAGE msg = { 0 };

    // Log BEFORE polling
    // LOG("LeapPoller::poll() - Attempting LeapPollConnection..."); 
#ifdef VERBOSE_LEAP_LOGGING
    static int pollLogCounter = 0;
    if (++pollLogCounter % 100 == 0) {
        OutputDebugStringA("LeapPoller::poll() - Attempting LeapPollConnection...\n");
    }
#endif

    eLeapRS result = LeapPollConnection(connection_, 30, &msg); // Poll once with timeout (Reduced from 100ms)
    if (result != eLeapRS_Success && result != eLeapRS_Timeout) { // Ignore timeout errors, log others
        std::cerr << "LeapPollConnection failed: " << GetLeapRSString(result) << std::endl;
        return; // Or handle error more robustly
    }

    // Log the received event type (always, including None)
    LOG("LeapPoller::poll() received event type: " + std::to_string(msg.type) + ", result: " + GetLeapRSString(result));

    switch (msg.type) {
    case eLeapEventType_None: // Expected on timeout
        break;
    case eLeapEventType_Connection:
        LOG("Leap Service Connected.");
        // Notify LeapInput if registered (connection event)
        if (leapInputCallback_) {
            leapInputCallback_->onLeapServiceConnect();
        }
        break;
    case eLeapEventType_Device:
        handleDeviceEvent(msg.device_event);
        break;
    case eLeapEventType_DeviceLost:
        handleDeviceLost(msg.device_event);
        break;
    case eLeapEventType_Tracking:
        if (msg.tracking_event) {
            // Use msg.device_id to find the correct device
            auto it = std::find_if(devices_.begin(), devices_.end(),
                [&](const DeviceInfo& info){ return info.id == msg.device_id; });
            if (it != devices_.end()) {
                std::string trackingDeviceSerial = it->serialNumber;
                handleTracking(msg.tracking_event, trackingDeviceSerial);
            } else {
                std::cerr << "[LeapPoller] Warning: Tracking event for unknown device id: " << msg.device_id << std::endl;
            }
        }
        break;
    case eLeapEventType_Policy:
        // Policy event: safe to access msg.policy_event
        LOG("Leap Policy changed. Flags: " << msg.policy_event->current_policy);
        // Add handling if needed, e.g., call a specific callback
        break;
    case eLeapEventType_DeviceStatusChange:
        if (msg.device_status_change_event) { // Check if the pointer is valid
            LOG("Leap Device Status Change Event for device ID: " << msg.device_status_change_event->device.id << ". Status Flags: " << msg.device_status_change_event->status);
            // You might want to update device state based on the status flags here
        } else {
            LOG_ERR("Received DeviceStatusChange event but pointer was null.");
        }
        break;
    // Add cases for other known events if necessary (e.g., LogEvent, DeviceStatusChange)
    default:
        // WARNING: Unknown event type. Do NOT access union members!
        std::cerr << "[LeapPoller] Unhandled/Unknown Leap Event Type: " << msg.type << std::endl;
        // Raw dump: print all pointer members for debugging
        std::cerr << "  msg.device_event=" << static_cast<const void*>(msg.device_event)
                  << ", msg.tracking_event=" << static_cast<const void*>(msg.tracking_event)
                  << ", msg.policy_event=" << static_cast<const void*>(msg.policy_event)
                  << ", msg.log_event=" << static_cast<const void*>(msg.log_event)
                  << ", msg.device_failure_event=" << static_cast<const void*>(msg.device_failure_event)
                  << ", msg.connection_event=" << static_cast<const void*>(msg.connection_event)
                  << ", msg.connection_lost_event=" << static_cast<const void*>(msg.connection_lost_event)
                  << std::endl;
        // If you want to dump more, add fields here, but NEVER access if you don't know the type!
        break;
    }
    // Check for disconnect event
    if (msg.type == eLeapEventType_ConnectionLost) {
        LOG("Leap Service Disconnected.");
        if (leapInputCallback_) {
            leapInputCallback_->onLeapServiceDisconnect();
        }
    }
}

const std::vector<LeapPoller::DeviceInfo>& LeapPoller::getDevices() const {
    return devices_;
}

void LeapPoller::setFrameCallback(FrameCallback cb) {
    frameCallback_ = std::move(cb);
}

void LeapPoller::cleanup()
{
    LOG("LeapPoller cleanup: Closing any remaining tracked device handles...");
    for (const auto& info : devices_) {
        if (info.deviceHandle) {
             LOG("Closing handle for SN: " + info.serialNumber);
             LeapUnsubscribeEvents(connection_, info.deviceHandle); // Ensure unsubscribe before close
             LeapCloseDevice(info.deviceHandle);
        }
    }
    devices_.clear(); // Clear the list after closing handles
}
