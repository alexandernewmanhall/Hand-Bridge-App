#pragma once
#include <string>
#include <map> // Needed for get/setAll HandAssignments

// Forward declaration
class DeviceAliasManager;

class ConfigManagerInterface {
public:
    virtual ~ConfigManagerInterface() = default;
    
    // Core config methods
    virtual bool loadConfig() = 0;
    virtual bool saveConfig() = 0;
    
    // Specific Settings
    virtual bool getLowLatencyMode() const = 0;
    virtual void setLowLatencyMode(bool enabled) = 0;
    virtual std::string getOscIp() const = 0;
    virtual int getOscPort() const = 0;
    virtual void setOscIp(const std::string& ip) = 0;
    virtual void setOscPort(int port) = 0;
    
    // Hand Assignments
    virtual std::string getDefaultHandAssignment(const std::string& serialNumber) const = 0;
    virtual void setDefaultHandAssignment(const std::string& serialNumber, const std::string& handType) = 0;
    virtual void setAllDefaultHandAssignments(const std::map<std::string, std::string>& assignments) = 0;
    virtual std::map<std::string, std::string> getAllDefaultHandAssignments() const = 0;

    // Device Aliases
    virtual DeviceAliasManager& getDeviceAliasManager() = 0;
    virtual const DeviceAliasManager& getDeviceAliasManager() const = 0;
    
    // REMOVED generic boolean settings methods
    // virtual bool getBool(const std::string& key, bool defaultValue) const = 0;
    // virtual void setBool(const std::string& key, bool value) = 0;

    // ADD specific pure virtual getters for filter flags
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

    // Add pinch/grab getters to interface
    virtual bool isSendPinchStrengthEnabled() const = 0;
    virtual bool isSendGrabStrengthEnabled() const = 0;

    // ADD specific pure virtual setters for filter flags
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

    // Add pinch/grab setters to interface
    virtual void setSendPinchStrengthEnabled(bool enabled) = 0;
    virtual void setSendGrabStrengthEnabled(bool enabled) = 0;
}; 