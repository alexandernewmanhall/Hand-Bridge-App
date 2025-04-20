#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Vector3 {
    float x = 0, y = 0, z = 0;
};

struct Quaternion {
    float w = 1, x = 0, y = 0, z = 0;
};

struct PalmData {
    Vector3 position;
    Vector3 velocity;
    Vector3 normal;
    Vector3 direction;
    Quaternion orientation;
    float width = 0;
};

struct ArmData {
    Vector3 wristPosition;
    Vector3 elbowPosition;
    float width = 0;
    Quaternion rotation;
    bool valid = true; // tests always intend a valid arm
    bool isValid() const { return valid; }
    void setValid(bool v) { valid = v; }
};

struct BoneData {
    Vector3 prevJoint;
    Vector3 nextJoint;
    float width = 0;
    Quaternion rotation;
    bool valid = true; // default to valid in tests
    bool isValid() const { return valid; }
    void setValid(bool v) { valid = v; }
};

struct FingerData {
    int fingerId = 0;
    bool isExtended = false;
    float extendedConfidence = 0;
    std::vector<BoneData> bones;
    bool valid = true; // NEW: default valid
    bool isValid() const { return valid; }
    void setValid(bool v) { valid = v; }
};

struct HandData {
    std::string handType; // "left" or "right"
    PalmData palm;
    ArmData arm;
    std::vector<FingerData> fingers;
    float pinchStrength = 0;
    float grabStrength = 0;
    float confidence = 0;
    uint64_t visibleTime = 0;
    bool valid = true;
    bool isValid() const { return valid; }
    void setValid(bool v) { valid = v; }
    // Add more fields as needed
};
