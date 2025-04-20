#include <gtest/gtest.h>
#include "../src/ui/DeviceTrackingModel.hpp"
#include <string>
#include <vector>

TEST(DeviceTrackingModelTest, ConnectAssignAndCount) {
    DeviceTrackingModel model;
    model.connectDevice("serialA");
    model.connectDevice("serialB");
    model.assignHand("serialA", "left");
    model.assignHand("serialB", "right");
    model.setHandCount("serialA", 1);
    model.setHandCount("serialB", 2);
    model.setFrameCount("serialA", 123);
    model.setFrameCount("serialB", 456);

    auto& devA = model.getDevice("serialA");
    auto& devB = model.getDevice("serialB");
    EXPECT_TRUE(devA.isConnected);
    EXPECT_TRUE(devB.isConnected);
    EXPECT_EQ(devA.assignedHand, "left");
    EXPECT_EQ(devB.assignedHand, "right");
    EXPECT_EQ(devA.handCount, 1);
    EXPECT_EQ(devB.handCount, 2);
    EXPECT_EQ(devA.frameCount, 123);
    EXPECT_EQ(devB.frameCount, 456);

    auto serials = model.getConnectedSerials();
    ASSERT_EQ(serials.size(), 2);
    EXPECT_NE(std::find(serials.begin(), serials.end(), "serialA"), serials.end());
    EXPECT_NE(std::find(serials.begin(), serials.end(), "serialB"), serials.end());
}
