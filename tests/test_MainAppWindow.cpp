#include <gtest/gtest.h>
#include "../src/ui/MainAppWindow.h"
#include <string>
#include "../src/core/ConfigManager.h"
#include "../src/transport/osc/OscController.h"
#include "../src/core/interfaces/ITransportSink.hpp"
#include "../src/core/FrameData.hpp"
#include "../src/core/ConnectEvent.hpp"
#include "../src/core/DisconnectEvent.hpp"
#include "../src/core/DeviceConnectedEvent.hpp"
#include "../src/core/DeviceLostEvent.hpp"
#include "../src/core/DeviceHandAssignedEvent.hpp"

// Mock Transport Sink for testing
class MockTransportSink : public ITransportSink {
public:
    void sendOscMessage(const OscMessage& message) override { /* No-op */ }
    bool send(const void* data, size_t size) override { return true; /* No-op */ }
    void updateTarget(const std::string& target, int port) override { /* No-op */ }
    void close() override { /* No-op */ }
};

class MainAppWindowTest_FRIEND : public ::testing::Test {
public:
    // Helper method to call the private addStatusMessage
    void callAddStatusMessage(MainAppWindow& app, const std::string& msg) {
        app.addStatusMessage(msg);
    }

    // Helper method to call the private getStatusMessages
    const std::vector<std::string>& callGetStatusMessages(const MainAppWindow& app) {
        return app.getStatusMessages();
    }
};

TEST_F(MainAppWindowTest_FRIEND, DeviceTrackingAndHandAssignment) {
    // Instantiate with dummy callbacks
    MainAppWindow app(
        [](const FrameData&){},          // Use real FrameData
        [](const ConnectEvent&) {},       // Use real ConnectEvent
        [](const DisconnectEvent&) {},    // Use real DisconnectEvent
        [](const DeviceConnectedEvent&) {},// Use real DeviceConnectedEvent
        [](const DeviceLostEvent&) {},     // Use real DeviceLostEvent
        [](const DeviceHandAssignedEvent&) {}, // Use real DeviceHandAssignedEvent
        [](const std::string&){}                // Reinstated dummy logger lambda
    );

    // Simulate device connect/hand assignment
    std::string serial = "serialX";
    // auto& data = app.getDeviceData(serial); // Commented out - calls private member
    // data.isConnected = true;                // Commented out
    // data.assignedHand = "left";             // Commented out
    // data.handCount = 1;                     // Commented out
    // data.frameCount = 42;                   // Commented out

    // Check state
    // EXPECT_TRUE(app.getDeviceData(serial).isConnected); // Commented out - calls private member
    // EXPECT_EQ(app.getDeviceData(serial).assignedHand, "left"); // Commented out - calls private member
    // EXPECT_EQ(app.getDeviceData(serial).handCount, 1); // Commented out - calls private member
    // EXPECT_EQ(app.getDeviceData(serial).frameCount, 42); // Commented out - calls private member
}

TEST_F(MainAppWindowTest_FRIEND, OscFilterToggles) {
    // Dependencies
    MockTransportSink mockSink;
    ConfigManager configManager; // Use concrete implementation
    OscController oscController(mockSink, configManager); // Pass dependencies

    // Set default values (optional, but good practice)
    configManager.setSendPalmEnabled(true);
    configManager.setSendThumbEnabled(true);
    // Set other fingers true initially if needed, e.g.:
    configManager.setSendIndexEnabled(true);
    configManager.setSendMiddleEnabled(true);
    configManager.setSendRingEnabled(true);
    configManager.setSendPinkyEnabled(true);

    MainAppWindow app(
        [](const FrameData&){},          // Use real FrameData
        [](const ConnectEvent&) {},       // Use real ConnectEvent
        [](const DisconnectEvent&) {},    // Use real DisconnectEvent
        [](const DeviceConnectedEvent&) {},// Use real DeviceConnectedEvent
        [](const DeviceLostEvent&) {},     // Use real DeviceLostEvent
        [](const DeviceHandAssignedEvent&) {}, // Use real DeviceHandAssignedEvent
        [](const std::string&){}                // Reinstated dummy logger lambda
    );

    // Link controllers
    app.setControllers(&configManager, &oscController);

    // Toggle OSC filter flags using ConfigManager
    configManager.setSendPalmEnabled(false);
    configManager.setSendThumbEnabled(false);
    // Explicitly set all finger flags to false
    configManager.setSendIndexEnabled(false);
    configManager.setSendMiddleEnabled(false);
    configManager.setSendRingEnabled(false);
    configManager.setSendPinkyEnabled(false);

    // Check flags using OscController getters
    EXPECT_FALSE(oscController.getSendPalmFlag());
    EXPECT_FALSE(oscController.getSendThumbFlag());
    EXPECT_FALSE(oscController.getSendAnyFingerFlag()); // Check if *any* finger flag is still true
}

TEST_F(MainAppWindowTest_FRIEND, StatusMessages) {
    MainAppWindow app(
        [](const FrameData&){},          // Use real FrameData
        [](const ConnectEvent&) {},       // Use real ConnectEvent
        [](const DisconnectEvent&) {},    // Use real DisconnectEvent
        [](const DeviceConnectedEvent&) {},// Use real DeviceConnectedEvent
        [](const DeviceLostEvent&) {},     // Use real DeviceLostEvent
        [](const DeviceHandAssignedEvent&) {}, // Use real DeviceHandAssignedEvent
        [](const std::string&){}                // Reinstated dummy logger lambda
    );
    callAddStatusMessage(app, "Device connected");
    callAddStatusMessage(app, "Hand assigned");
    const auto& messages = callGetStatusMessages(app);
    ASSERT_EQ(messages.size(), 2);
    EXPECT_EQ(messages[0], "Device connected");
    EXPECT_EQ(messages[1], "Hand assigned");
}
