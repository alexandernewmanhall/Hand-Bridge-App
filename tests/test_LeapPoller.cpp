#include <gtest/gtest.h>
#include <vector>
#include <string>

// Minimal mock device manager for unit testing
class MockDeviceManager {
public:
    struct DeviceInfo {
        int id;
        std::string serialNumber;
    };
    std::vector<DeviceInfo> devices;
    std::vector<std::string> connectedSerials;

    void connectDevice(int id, const std::string& serial) {
        devices.push_back({id, serial});
        connectedSerials.push_back(serial);
    }
};

TEST(MockDeviceManagerTest, TracksMultipleDevices) {
    MockDeviceManager manager;
    manager.connectDevice(1, "serialA");
    manager.connectDevice(2, "serialB");
    ASSERT_EQ(manager.devices.size(), 2);
    EXPECT_EQ(manager.devices[0].serialNumber, "serialA");
    EXPECT_EQ(manager.devices[1].serialNumber, "serialB");
    ASSERT_EQ(manager.connectedSerials.size(), 2);
    EXPECT_EQ(manager.connectedSerials[0], "serialA");
    EXPECT_EQ(manager.connectedSerials[1], "serialB");
}
