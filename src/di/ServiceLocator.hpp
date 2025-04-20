#pragma once

#include <memory>           // For std::shared_ptr
#include <unordered_map>    // For std::unordered_map
#include <typeindex>        // For std::type_index
#include <stdexcept>        // For std::runtime_error
#include <string>
#include <sstream>          // For building error messages

class ServiceLocator {
public:
    // Registers a service instance.
    // Throws std::runtime_error if a service of the same type is already registered.
    template<typename T>
    void add(std::shared_ptr<T> service) {
        auto typeIndex = std::type_index(typeid(T));
        if (services_.count(typeIndex)) {
            std::string errorMsg = "Service of type '" + std::string(typeid(T).name()) + "' already registered.";
            throw std::runtime_error(errorMsg);
        }
        services_[typeIndex] = service; // Implicit cast to shared_ptr<void>
    }

    // Retrieves a registered service instance.
    // Throws std::runtime_error if the service type is not found.
    template<typename T>
    std::shared_ptr<T> get() const {
        auto typeIndex = std::type_index(typeid(T));
        auto it = services_.find(typeIndex);
        if (it == services_.end()) {
            std::string errorMsg = "Service of type '" + std::string(typeid(T).name()) + "' not found.";
            throw std::runtime_error(errorMsg);
        }
        // Cast back from shared_ptr<void> to shared_ptr<T>
        return std::static_pointer_cast<T>(it->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
};