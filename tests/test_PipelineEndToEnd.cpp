#include <gtest/gtest.h>
#include "../src/pipeline/03_DataProcessor.hpp"
#include "../src/core/FrameData.hpp"
#include "../src/core/DeviceAliasManager.hpp"
#include <vector>
#include <string>
#include <algorithm>

// Mock OscSender that records messages instead of sending them
class MockOscSender {
public:
    std::vector<std::string> sentAddresses;
    void sendMessage(const std::string& address, float value) {
        sentAddresses.push_back(address);
    }
};

TEST(PipelineEndToEnd, FrameToOscMessage) {
    DeviceAliasManager aliasMgr;
    MockOscSender oscSender;

    // DataProcessor with lambda that calls our mock sender
    auto logger = std::make_shared<AppLogger>();
    DataProcessor processor(
        aliasMgr,
        [&](const OscMessage& msg) {
            for (float v : msg.values)
                oscSender.sendMessage(msg.address, v);
        },
        [](const FrameData&) {},
        logger
    );

    // Enable palm data only
    processor.setFilterSettings(true, false, false, false, false, false, false, false, false, false, false, false, true, true);

    // Create synthetic FrameData
    FrameData frame;
    frame.deviceId = "serialA";
    HandData hand;
    hand.handType = "left";
    hand.palm.position = {1, 2, 3};
    frame.hands.push_back(hand);

    // Simulate pipeline: DataProcessor is usually called by LeapSorter, etc.
    processor.processData("serialA", frame);

    // Assert that the correct OSC addresses were generated
    ASSERT_FALSE(oscSender.sentAddresses.empty());
    EXPECT_NE(std::find(oscSender.sentAddresses.begin(), oscSender.sentAddresses.end(), "/leap/dev1/left/palm/tx"), oscSender.sentAddresses.end());
    EXPECT_NE(std::find(oscSender.sentAddresses.begin(), oscSender.sentAddresses.end(), "/leap/dev1/left/palm/ty"), oscSender.sentAddresses.end());
    EXPECT_NE(std::find(oscSender.sentAddresses.begin(), oscSender.sentAddresses.end(), "/leap/dev1/left/palm/tz"), oscSender.sentAddresses.end());
}

// Multi-device test
TEST(PipelineEndToEnd, MultiDeviceFrameToOscMessage) {
    DeviceAliasManager aliasMgr;
    MockOscSender oscSender;
    auto logger2 = std::make_shared<AppLogger>();
    DataProcessor processor(
        aliasMgr,
        [&](const OscMessage& msg) {
            // Prefix serial to address for test verification
            oscSender.sendMessage(msg.address, msg.values.empty() ? 0.0f : msg.values[0]);
        },
        [](const FrameData&) {},
        logger2
    );
    processor.setFilterSettings(true, false, false, false, false, false, false, false, false, false, false, true, true, true);

    // Device 1
    FrameData frame1;
    frame1.deviceId = "serialA";
    HandData hand1; hand1.handType = "left"; hand1.palm.position = {1, 2, 3};
    frame1.hands.push_back(hand1);

    // Device 2
    FrameData frame2;
    frame2.deviceId = "serialB";
    HandData hand2; hand2.handType = "right"; hand2.palm.position = {4, 5, 6};
    frame2.hands.push_back(hand2);

    processor.processData("serialA", frame1);
    processor.processData("serialB", frame2);

    // Check that both serials/addresses are present in the outputs
    bool foundA = false, foundB = false;
    for (const auto& addr : oscSender.sentAddresses) {
        if (addr.find("serialA") != std::string::npos) foundA = true;
        if (addr.find("serialB") != std::string::npos) foundB = true;
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);
}
