#include "ConfigManager.h"
#include "json.hpp" // Use nlohmann::json
#include "Log.hpp"
#include <fstream>
#include <filesystem> // For path manipulation
#include <shlobj.h>   // For SHGetFolderPath
#include <windows.h>  // For PWSTR
#include <KnownFolders.h> // For FOLDERID_LocalAppData
#include <iomanip> // For std::setw

using json = nlohmann::json;

// Helper function to get %LOCALAPPDATA% path (Windows specific)
std::string getLocalAppDataPath() {
    PWSTR path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
    std::string appDataPath;
    if (SUCCEEDED(hr)) {
        // Convert WCHAR* to std::string (assuming UTF-8 is okay here)
        // For robust conversion, consider WideCharToMultiByte
        std::wstring widePath(path);
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &widePath[0], (int)widePath.size(), NULL, 0, NULL, NULL);
        std::string narrowPath(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &widePath[0], (int)widePath.size(), &narrowPath[0], size_needed, NULL, NULL);
        appDataPath = narrowPath;
    } else {
        // Fallback or error handling
        LOG_ERR("Failed to get LocalAppData path.");
    }
    CoTaskMemFree(path);
    return appDataPath;
}

// Constructor - Initialize members with defaults
ConfigManager::ConfigManager()
    : baseGain_(1.0f), midGain_(1.0f), maxGain_(1.0f), lowSpeedThreshold_(0.0f), midSpeedThreshold_(0.0f)
{
    // Constructor body if needed
}

float ConfigManager::getBaseGain() const { return baseGain_; }
float ConfigManager::getMidGain() const { return midGain_; }
float ConfigManager::getMaxGain() const { return maxGain_; }
float ConfigManager::getLowSpeedThreshold() const { return lowSpeedThreshold_; }
float ConfigManager::getMidSpeedThreshold() const { return midSpeedThreshold_; }
void ConfigManager::setGainParams(float base, float mid, float max, float lowThresh, float midThresh) {
    baseGain_ = base;
    midGain_ = mid;
    maxGain_ = max;
    lowSpeedThreshold_ = lowThresh;
    midSpeedThreshold_ = midThresh;
}

// Get config path helper (moved inside)
std::filesystem::path getConfigPath() {
    std::string localAppData = getLocalAppDataPath();
    if (localAppData.empty()) {
        LOG_ERR("Could not determine LocalAppData path. Config saving/loading might fail.");
        return {}; // Return empty path on failure
    }
    return std::filesystem::path(localAppData) / "LeapApp" / "config.json";
}

// Load configuration implementation (public, no args)
bool ConfigManager::loadConfig() {
    return loadConfig(getConfigPath().string());
}

// Load configuration implementation (public, with filename)
bool ConfigManager::loadConfig(const std::string& filename) {
     if (filename.empty()) return false;
     std::ifstream ifs(filename);
     if (!ifs.is_open()) {
         LOG("Config file not found: " << filename << ". Using defaults.");
         return false; // File doesn't exist, use defaults
     }

    try {
        json j;
        ifs >> j;

        // Load OSC settings
        this->oscIp = j.value("osc_ip", "127.0.0.1");
        this->oscPort = j.value("osc_port", 9000);
        this->lowLatencyMode = j.value("low_latency_mode", false);

        // Load Hand Assignments
        if (j.contains("hand_assignments") && j["hand_assignments"].is_object()) {
            this->deviceHandAssignments = j["hand_assignments"].get<std::map<std::string, std::string>>();
        }

        // Load Aliases
        if (j.contains("device_aliases") && j["device_aliases"].is_object()) {
            deviceAliasManager.loadFromJson(j["device_aliases"]);
        }

        // Load Filter Settings using specific members
        if (j.contains("booleanSettings") && j["booleanSettings"].is_object()) {
            auto& settings = j["booleanSettings"];
            this->sendPalm_ = settings.value("sendPalm", this->sendPalm_);
            this->sendWrist_ = settings.value("sendWrist", this->sendWrist_);
            this->sendThumb_ = settings.value("sendThumb", this->sendThumb_);
            this->sendIndex_ = settings.value("sendIndex", this->sendIndex_);
            this->sendMiddle_ = settings.value("sendMiddle", this->sendMiddle_);
            this->sendRing_ = settings.value("sendRing", this->sendRing_);
            this->sendPinky_ = settings.value("sendPinky", this->sendPinky_);
            this->sendPalmOrientation_ = settings.value("sendPalmOrientation", this->sendPalmOrientation_);
            this->sendPalmVelocity_ = settings.value("sendPalmVelocity", this->sendPalmVelocity_);
            this->sendPalmNormal_ = settings.value("sendPalmNormal", this->sendPalmNormal_);
            this->sendVisibleTime_ = settings.value("sendVisibleTime", this->sendVisibleTime_);
            this->sendFingerIsExtended_ = settings.value("sendFingerIsExtended", this->sendFingerIsExtended_);
            this->sendPinchStrength_ = settings.value("sendPinchStrength", this->sendPinchStrength_);
            this->sendGrabStrength_ = settings.value("sendGrabStrength", this->sendGrabStrength_);
        }

        LOG("Configuration loaded successfully from " << filename);
        return true;
    } catch (const json::exception& e) {
        (void)e;
        LOG_ERR("Error parsing config file " << filename << ": " << e.what());
        return false;
    }
}

// Save configuration implementation (public, no args)
bool ConfigManager::saveConfig() {
    // Calls the IConfigStore::save override below
    return save(getConfigPath().string()); 
}

// Private helper definition (renamed and const)
bool ConfigManager::saveConfigToFile(const std::string& filename) const { 
    if (filename.empty()) {
        LOG_ERR("Config filename is empty, cannot save.");
        return false;
    }
    // Ensure directory exists
    std::filesystem::path configPath(filename);
    std::filesystem::path configDir = configPath.parent_path();
    try {
        if (!std::filesystem::exists(configDir)) {
            std::filesystem::create_directories(configDir);
            LOG("Created config directory: " << configDir.string());
        }
    } catch (const std::filesystem::filesystem_error& e) {
        (void)e;
        LOG_ERR("Error creating config directory " << configDir.string() << ": " << e.what());
        return false;
    }

    json j;
    // Save OSC settings
    j["osc_ip"] = this->oscIp; 
    j["osc_port"] = this->oscPort;
    j["low_latency_mode"] = this->lowLatencyMode;
    // Save Hand Assignments
    j["hand_assignments"] = this->deviceHandAssignments;
    // Save Aliases
    j["device_aliases"] = deviceAliasManager.toJson()["device_aliases"];
    // Save Filter Settings
    json booleanSettings;
    booleanSettings["sendPalm"] = this->sendPalm_;
    booleanSettings["sendWrist"] = this->sendWrist_;
    booleanSettings["sendThumb"] = this->sendThumb_;
    booleanSettings["sendIndex"] = this->sendIndex_;
    booleanSettings["sendMiddle"] = this->sendMiddle_;
    booleanSettings["sendRing"] = this->sendRing_;
    booleanSettings["sendPinky"] = this->sendPinky_;
    booleanSettings["sendPalmOrientation"] = this->sendPalmOrientation_;
    booleanSettings["sendPalmVelocity"] = this->sendPalmVelocity_;
    booleanSettings["sendPalmNormal"] = this->sendPalmNormal_;
    booleanSettings["sendVisibleTime"] = this->sendVisibleTime_;
    booleanSettings["sendFingerIsExtended"] = this->sendFingerIsExtended_;
    booleanSettings["sendPinchStrength"] = this->sendPinchStrength_;
    booleanSettings["sendGrabStrength"] = this->sendGrabStrength_;
    j["booleanSettings"] = booleanSettings;

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        LOG_ERR("Could not open config file for writing: " << filename);
        return false;
    }
    try {
        ofs << std::setw(4) << j << std::endl; 
        LOG("Configuration saved successfully to " << filename);
        return true;
    } catch (const std::exception& e) {
        (void)e;
        LOG_ERR("Error writing config file " << filename << ": " << e.what());
        return false;
    }
}

// Device preference implementations
void ConfigManager::setDefaultHandAssignment(const std::string& serialNumber, const std::string& hand) {
    if (hand.empty() || hand == "NONE") {
        this->deviceHandAssignments.erase(serialNumber);
    } else {
        this->deviceHandAssignments[serialNumber] = hand;
    }
}

void ConfigManager::setAllDefaultHandAssignments(const std::map<std::string, std::string>& assignments) {
    this->deviceHandAssignments = assignments;
}

std::string ConfigManager::getDefaultHandAssignment(const std::string& serialNumber) const {
    auto it = this->deviceHandAssignments.find(serialNumber);
    if (it != this->deviceHandAssignments.end()) {
        return it->second;
    }
    return ""; // Return empty string if not found
}

std::map<std::string, std::string> ConfigManager::getAllDefaultHandAssignments() const {
    return this->deviceHandAssignments;
}

DeviceAliasManager& ConfigManager::getDeviceAliasManager() { return deviceAliasManager; }
const DeviceAliasManager& ConfigManager::getDeviceAliasManager() const { return deviceAliasManager; }

// Implement IConfigStore pure virtuals
bool ConfigManager::load(const std::string& filename) {
    return loadConfig(filename);
}
// IConfigStore::save override (must be const)
bool ConfigManager::save(const std::string& filename) const {
    // Calls the const helper method defined above
    return saveConfigToFile(filename); 
}


std::string ConfigManager::getOscIp() const { return oscIp; }
int ConfigManager::getOscPort() const { return oscPort; }
void ConfigManager::setOscIp(const std::string& ip) { oscIp = ip; }
void ConfigManager::setOscPort(int port) { oscPort = port; }

bool ConfigManager::getLowLatencyMode() const { return lowLatencyMode; }
void ConfigManager::setLowLatencyMode(bool enabled) { lowLatencyMode = enabled; }

bool ConfigManager::isSendPalmEnabled() const { return sendPalm_; }
bool ConfigManager::isSendWristEnabled() const { return sendWrist_; }
bool ConfigManager::isSendThumbEnabled() const { return sendThumb_; }
bool ConfigManager::isSendIndexEnabled() const { return sendIndex_; }
bool ConfigManager::isSendMiddleEnabled() const { return sendMiddle_; }
bool ConfigManager::isSendRingEnabled() const { return sendRing_; }
bool ConfigManager::isSendPinkyEnabled() const { return sendPinky_; }
bool ConfigManager::isSendPalmOrientationEnabled() const { return sendPalmOrientation_; }
bool ConfigManager::isSendPalmVelocityEnabled() const { return sendPalmVelocity_; }
bool ConfigManager::isSendPalmNormalEnabled() const { return sendPalmNormal_; }
bool ConfigManager::isSendVisibleTimeEnabled() const { return sendVisibleTime_; }
bool ConfigManager::isSendFingerIsExtendedEnabled() const { return sendFingerIsExtended_; }
bool ConfigManager::isSendPinchStrengthEnabled() const { return sendPinchStrength_; }
bool ConfigManager::isSendGrabStrengthEnabled() const { return sendGrabStrength_; }

void ConfigManager::setSendPalmEnabled(bool enabled) { sendPalm_ = enabled; }
void ConfigManager::setSendWristEnabled(bool enabled) { sendWrist_ = enabled; }
void ConfigManager::setSendThumbEnabled(bool enabled) { sendThumb_ = enabled; }
void ConfigManager::setSendIndexEnabled(bool enabled) { sendIndex_ = enabled; }
void ConfigManager::setSendMiddleEnabled(bool enabled) { sendMiddle_ = enabled; }
void ConfigManager::setSendRingEnabled(bool enabled) { sendRing_ = enabled; }
void ConfigManager::setSendPinkyEnabled(bool enabled) { sendPinky_ = enabled; }
void ConfigManager::setSendPalmOrientationEnabled(bool enabled) { sendPalmOrientation_ = enabled; }
void ConfigManager::setSendPalmVelocityEnabled(bool enabled) { sendPalmVelocity_ = enabled; }
void ConfigManager::setSendPalmNormalEnabled(bool enabled) { sendPalmNormal_ = enabled; }
void ConfigManager::setSendVisibleTimeEnabled(bool enabled) { sendVisibleTime_ = enabled; }
void ConfigManager::setSendFingerIsExtendedEnabled(bool enabled) { sendFingerIsExtended_ = enabled; }
void ConfigManager::setSendPinchStrengthEnabled(bool enabled) { sendPinchStrength_ = enabled; }
void ConfigManager::setSendGrabStrengthEnabled(bool enabled) { sendGrabStrength_ = enabled; }