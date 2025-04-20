#include <gtest/gtest.h>
#include "../src/pipeline/02_LeapSorter.hpp"
#include <vector>
#include <string>

TEST(LeapSorterTest, CallsCallbackForEachDevice) {
    using CallbackRecord = std::pair<std::string, FrameData>;
    std::vector<CallbackRecord> callbackResults;

    // Mock callback that records serial and frame
    auto callback = [&callbackResults](const std::string& serial, const FrameData& frame) {
        callbackResults.emplace_back(serial, frame);
    };

    LeapSorter sorter(callback);

    // Create dummy frames
    FrameData frame1; frame1.deviceId = "serialA"; frame1.timestamp = 123;
    FrameData frame2; frame2.deviceId = "serialB"; frame2.timestamp = 456;

    // Process frames for two devices
    sorter.processFrame("serialA", frame1);
    sorter.processFrame("serialB", frame2);

    // Assert callback was called for both
    ASSERT_EQ(callbackResults.size(), 2);
    EXPECT_EQ(callbackResults[0].first, "serialA");
    EXPECT_EQ(callbackResults[0].second.timestamp, 123);
    EXPECT_EQ(callbackResults[1].first, "serialB");
    EXPECT_EQ(callbackResults[1].second.timestamp, 456);
}
