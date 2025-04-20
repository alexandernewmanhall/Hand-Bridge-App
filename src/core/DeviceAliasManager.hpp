#pragma once
#include <string>
#include <map>
#include <mutex>
#include "json.hpp"

/**
 * @brief Manages device aliases for tracking multiple Leap Motion controllers
 * 
 * This class maintains a mapping between device serial numbers and unique aliases
 * that can be used to identify devices across sessions.
 */
enum class AssignedHand {
    None,
    Left,
    Right,
    Both
};

class DeviceAliasManager {
public:
    /**
     * @brief Gets or assigns an alias for a device serial number
     * 
     * If the serial number already has an alias, returns it. Otherwise, generates
     * a new unique alias and returns it.
     * 
     * @param serial The device serial number
     * @return A unique alias for the device
     */
    std::string getOrAssignAlias(const std::string& serial);
    // Compatibility: lookupAlias is an alias for getOrAssignAlias
    inline std::string lookupAlias(const std::string& serial) { return getOrAssignAlias(serial); }

    /**
     * @brief Loads device alias mappings from a JSON object
     * 
     * Clears existing mappings and loads new ones from the provided JSON.
     * 
     * @param j JSON object containing device alias mappings
     */
    void loadFromJson(const nlohmann::json& j);

    /**
     * @brief Serializes device alias mappings to a JSON object
     * 
     * @return JSON object containing all device alias mappings
     */
    nlohmann::json toJson() const;

    /**
     * @brief Clears all device alias mappings
     */
    void clear();

public:
    // Returns the hand assignment for the given alias (default: Both)
    AssignedHand getAssignedHand(const std::string& alias) const;

private:
    std::map<std::string, std::string> serialToAlias_;
    int nextAliasIndex_ = 1;
    mutable std::mutex mutex_;

    /**
     * @brief Validates a device alias format
     * 
     * @param alias The alias to validate
     * @throws std::invalid_argument If the alias is invalid
     */
    void validateAlias(const std::string& alias);

    std::string generateAlias();
};
