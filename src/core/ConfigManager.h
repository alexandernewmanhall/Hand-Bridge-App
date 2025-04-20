#pragma once

#include "interfaces/IConfigStore.hpp"
#include "ConfigManagerInterface.h"
#include <string>
#include <map>
#include <fstream>
#include <memory>
#include <vector>

// ConfigManager class for handling application configuration
// This class is now a regular DI-friendly implementation (no singleton)
#include "DeviceAliasManager.hpp"

class ConfigManager : public IConfigStore, public ConfigManagerInterface {
public:
    // Gain curve (KEEP THESE)
    float getBaseGain() const;
    float getMidGain() const;
    float getMaxGain() const;
    float getLowSpeedThreshold() const;
    float getMidSpeedThreshold() const;
    void setGainParams(float base, float mid, float max, float lowThresh, float midThresh);

    /**
     * DeviceAliasManager manages serial-to-alias mapping for all connected Leap devices.
     * This mapping is persisted in the config file and provides the single source of truth for device aliases.
     */
    // Public constructor for DI
    ConfigManager();

    // Delete copy constructor and assignment operator (still not copyable)
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // File IO
    bool load(const std::string& filename) override;
    bool save(const std::string& filename) const override;
    bool loadConfig(const std::string& filename);
    bool loadConfig() override;
    bool saveConfig() override;

    // OSC settings
    std::string getOscIp() const override;
    int getOscPort() const override;
    void setOscIp(const std::string& ip) override;
    void setOscPort(int port) override;

    // Low latency
    bool getLowLatencyMode() const override;
    void setLowLatencyMode(bool enabled) override;

    // Hand Assignments
    std::string getDefaultHandAssignment(const std::string& serialNumber) const override;
    void setDefaultHandAssignment(const std::string& serialNumber, const std::string& handType) override;
    void setAllDefaultHandAssignments(const std::map<std::string, std::string>& assignments) override;
    std::map<std::string, std::string> getAllDefaultHandAssignments() const override;

    // Device Aliases
    DeviceAliasManager& getDeviceAliasManager() override;
    const DeviceAliasManager& getDeviceAliasManager() const override;

    // Feature flags (getters)
    bool isSendPalmEnabled() const override;
    bool isSendWristEnabled() const override;
    bool isSendThumbEnabled() const override;
    bool isSendIndexEnabled() const override;
    bool isSendMiddleEnabled() const override;
    bool isSendRingEnabled() const override;
    bool isSendPinkyEnabled() const override;
    bool isSendPalmOrientationEnabled() const override;
    bool isSendPalmVelocityEnabled() const override;
    bool isSendPalmNormalEnabled() const override;
    bool isSendVisibleTimeEnabled() const override;
    bool isSendFingerIsExtendedEnabled() const override;
    bool isSendPinchStrengthEnabled() const override;
    bool isSendGrabStrengthEnabled() const override;

    // Feature flags (setters)
    void setSendPalmEnabled(bool enabled) override;
    void setSendWristEnabled(bool enabled) override;
    void setSendThumbEnabled(bool enabled) override;
    void setSendIndexEnabled(bool enabled) override;
    void setSendMiddleEnabled(bool enabled) override;
    void setSendRingEnabled(bool enabled) override;
    void setSendPinkyEnabled(bool enabled) override;
    void setSendPalmOrientationEnabled(bool enabled) override;
    void setSendPalmVelocityEnabled(bool enabled) override;
    void setSendPalmNormalEnabled(bool enabled) override;
    void setSendVisibleTimeEnabled(bool enabled) override;
    void setSendFingerIsExtendedEnabled(bool enabled) override;
    void setSendPinchStrengthEnabled(bool enabled) override;
    void setSendGrabStrengthEnabled(bool enabled) override;

private:
    // Gain curve members (KEEP THESE)
    float baseGain_ = 1.0f; 
    float midGain_ = 3.0f; 
    float maxGain_ = 6.0f;
    float lowSpeedThreshold_ = 80.0f; 
    float midSpeedThreshold_ = 240.0f;
    // Configuration storage
    std::string oscIp;
    int oscPort;
    bool lowLatencyMode;
    std::map<std::string, std::string> deviceHandAssignments;
    
    // Explicit boolean members for filters
    bool sendPalm_;
    bool sendWrist_;
    bool sendThumb_;
    bool sendIndex_;
    bool sendMiddle_;
    bool sendRing_;
    bool sendPinky_;
    bool sendPalmOrientation_;
    bool sendPalmVelocity_;
    bool sendPalmNormal_;
    bool sendVisibleTime_;
    bool sendFingerIsExtended_;

    // Add members for pinch/grab
    bool sendPinchStrength_;
    bool sendGrabStrength_;

    // Serial-to-alias mapping
    DeviceAliasManager deviceAliasManager;

    // Declare the const helper for saving
    bool saveConfigToFile(const std::string& filename) const;
}; 