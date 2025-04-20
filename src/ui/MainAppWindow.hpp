#include <string>
#include <vector>
#include <map>
#include <SDL.h>
#include <mutex>
#include "../core/DeviceConnectedEvent.hpp" // Include event struct
#include "../core/DeviceLostEvent.hpp" // Include event struct
#include "../core/DeviceHandAssignedEvent.hpp"
#include "../core/FrameData.hpp"
#include "UIController.hpp" // Include UIController


struct PerDeviceTrackingData {
    bool isConnected = false;
// ... existing code ...
    UIController* uiController_ = nullptr; // Pointer to the UIController

    // Callback for handling frame data updates from LeapInput/AppCore
    void handleTrackingData(const std::string& deviceId, const FrameData& frameData);
// ... existing code ...

    // Method to set the UIController
    void setUIController(UIController* controller);

private:
// ... existing code ...
} 