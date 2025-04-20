#pragma once

#include <string>
#include <map>

// Abstract interface for config file read/write
class DeviceAliasManager;

class IConfigStore {
public:
    virtual ~IConfigStore() = default;
    // File IO
    virtual bool load(const std::string& filename) = 0;
    virtual bool save(const std::string& filename) const = 0;
    virtual bool loadConfig() = 0;
    virtual bool saveConfig() = 0;

    // OSC settings
    virtual std::string getOscIp() const = 0;
    virtual int getOscPort() const = 0;
    virtual void setOscIp(const std::string& ip) = 0;
    virtual void setOscPort(int port) = 0;

    // Low latency
    virtual bool getLowLatencyMode() const = 0;
    virtual void setLowLatencyMode(bool enabled) = 0;

    // Hand Assignments
    virtual std::string getDefaultHandAssignment(const std::string& serialNumber) const = 0;
    virtual void setDefaultHandAssignment(const std::string& serialNumber, const std::string& handType) = 0;
    virtual void setAllDefaultHandAssignments(const std::map<std::string, std::string>& assignments) = 0;
    virtual std::map<std::string, std::string> getAllDefaultHandAssignments() const = 0;

    // Device Aliases
    virtual DeviceAliasManager& getDeviceAliasManager() = 0;
    virtual const DeviceAliasManager& getDeviceAliasManager() const = 0;

    // Feature flags (getters)
    virtual bool isSendPalmEnabled() const = 0;
    virtual bool isSendWristEnabled() const = 0;
    virtual bool isSendThumbEnabled() const = 0;
    virtual bool isSendIndexEnabled() const = 0;
    virtual bool isSendMiddleEnabled() const = 0;
    virtual bool isSendRingEnabled() const = 0;
    virtual bool isSendPinkyEnabled() const = 0;
    virtual bool isSendPalmOrientationEnabled() const = 0;
    virtual bool isSendPalmVelocityEnabled() const = 0;
    virtual bool isSendPalmNormalEnabled() const = 0;
    virtual bool isSendVisibleTimeEnabled() const = 0;
    virtual bool isSendFingerIsExtendedEnabled() const = 0;
    virtual bool isSendPinchStrengthEnabled() const = 0;
    virtual bool isSendGrabStrengthEnabled() const = 0;

    // Feature flags (setters)
    virtual void setSendPalmEnabled(bool enabled) = 0;
    virtual void setSendWristEnabled(bool enabled) = 0;
    virtual void setSendThumbEnabled(bool enabled) = 0;
    virtual void setSendIndexEnabled(bool enabled) = 0;
    virtual void setSendMiddleEnabled(bool enabled) = 0;
    virtual void setSendRingEnabled(bool enabled) = 0;
    virtual void setSendPinkyEnabled(bool enabled) = 0;
    virtual void setSendPalmOrientationEnabled(bool enabled) = 0;
    virtual void setSendPalmVelocityEnabled(bool enabled) = 0;
    virtual void setSendPalmNormalEnabled(bool enabled) = 0;
    virtual void setSendVisibleTimeEnabled(bool enabled) = 0;
    virtual void setSendFingerIsExtendedEnabled(bool enabled) = 0;
    virtual void setSendPinchStrengthEnabled(bool enabled) = 0;
    virtual void setSendGrabStrengthEnabled(bool enabled) = 0;
};
