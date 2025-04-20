#include "DeviceAliasManager.hpp"

AssignedHand DeviceAliasManager::getAssignedHand(const std::string& alias) const {
    // TODO: Replace with actual assignment logic if available
    // For now, always return Both (default)
    return AssignedHand::Both;
}

#include <sstream>
#include <stdexcept>
#include <string_view>
#include <iostream>
#include <mutex>
#include <memory>
#include <thread>
#include <shared_mutex>

void DeviceAliasManager::validateAlias(const std::string& alias) {
    if (alias.empty() || alias.size() < 4 || alias.substr(0, 3) != "dev") {
        throw std::invalid_argument("Invalid alias format: must start with 'dev' followed by a number");
    }
    try {
        size_t pos;
        int number = std::stoi(alias.substr(3), &pos); // Fix C4834: use return value
        (void)number; // Silence unused variable warning if not used
        if (pos != alias.size() - 3) {
            throw std::invalid_argument("Invalid alias format: number part contains non-numeric characters");
        }
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid alias format: number part is not a valid integer");
    }
}

std::string DeviceAliasManager::getOrAssignAlias(const std::string& serial) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = serialToAlias_.find(serial);
    if (it != serialToAlias_.end()) {
        return it->second;
    }
    std::string alias = generateAlias();
    validateAlias(alias);
    serialToAlias_[serial] = alias;
    ++nextAliasIndex_;
    return alias;
}

void DeviceAliasManager::loadFromJson(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    serialToAlias_.clear();
    if (j.contains("deviceAliases") && j["deviceAliases"].is_object()) {
        for (auto it = j["deviceAliases"].begin(); it != j["deviceAliases"].end(); ++it) {
            try {
                validateAlias(it.value());
                serialToAlias_[it.key()] = it.value();
            } catch (const std::exception& e) {
                // Log error but continue loading other valid aliases
                std::cerr << "Warning: Invalid alias format for device " << it.key() << ": " << e.what() << std::endl;
            }
        }
    }
    // Recalculate nextAliasIndex_
    nextAliasIndex_ = 1;
    for (const auto& pair : serialToAlias_) {
        try {
            if (pair.second.rfind("dev", 0) == 0) {
                int idx = std::stoi(pair.second.substr(3));
                if (idx >= nextAliasIndex_) nextAliasIndex_ = idx + 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to parse alias index for device " << pair.first << ": " << e.what() << std::endl;
        }
    }
}

nlohmann::json DeviceAliasManager::toJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    j["deviceAliases"] = serialToAlias_;
    return j;
}

void DeviceAliasManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    serialToAlias_.clear();
    nextAliasIndex_ = 1;
}

std::string DeviceAliasManager::generateAlias() {
    std::ostringstream oss;
    oss << "dev" << nextAliasIndex_;
    return oss.str();
}
