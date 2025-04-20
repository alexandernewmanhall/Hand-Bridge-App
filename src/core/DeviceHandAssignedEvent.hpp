#pragma once
#include <string>
#include <cstdint>
#include <chrono>

/**
 * @brief Represents a hand assignment event for a Leap Motion device
 * 
 * This event is triggered when a device is assigned to track a specific hand
 * (left or right) or when its hand assignment is cleared.
 */
struct DeviceHandAssignedEvent {
    /**
     * @brief The type of hand assignment
     */
    enum class HandType {
        HAND_LEFT,   /**< Assigned to track left hand */
        HAND_RIGHT,  /**< Assigned to track right hand */
        HAND_NONE    /**< Hand assignment cleared */
    };

    /**
     * @brief The device's serial number
     */
    std::string serialNumber;

    /**
     * @brief The assigned hand type
     */
    HandType handType;

    /**
     * @brief Timestamp of when the event occurred
     */
    std::chrono::system_clock::time_point timestamp;

    /**
     * @brief Creates a new DeviceHandAssignedEvent
     * 
     * @param serialNumber The device's serial number
     * @param handType The assigned hand type
     */
    DeviceHandAssignedEvent()
        : serialNumber(), handType(HandType::HAND_NONE), timestamp(std::chrono::system_clock::now()) {}

    DeviceHandAssignedEvent(const std::string& serialNumber, HandType handType)
        : serialNumber(serialNumber), handType(handType),
          timestamp(std::chrono::system_clock::now()) {}

    // Helper: Convert HandType to string
    static std::string handTypeToString(HandType ht) {
        switch (ht) {
            case HandType::HAND_LEFT: return "LEFT";
            case HandType::HAND_RIGHT: return "RIGHT";
            case HandType::HAND_NONE: return "NONE";
        }
        return "NONE";
    }

    // Helper: Convert string to HandType
    static HandType stringToHandType(const std::string& s) {
        if (s == "LEFT") return HandType::HAND_LEFT;
        if (s == "RIGHT") return HandType::HAND_RIGHT;
        return HandType::HAND_NONE;
    }
};
