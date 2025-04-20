#include <gtest/gtest.h>
#include "../src/pipeline/03_DataProcessor.hpp"
#include "../src/core/FrameData.hpp"
#include "../src/core/DeviceAliasManager.hpp"
#include "../src/core/AppLogger.hpp"
#include <memory>

// Utility: Create a valid hand for testing
HandData makeHand(const std::string& side) {
    HandData h;
    h.handType = side;
    h.palm.position = {1.0f, 2.0f, 3.0f};
    h.arm.wristPosition = {4.0f, 5.0f, 6.0f};
    h.arm.setValid(true);
    h.fingers.clear();
    for (int i = 0; i < 5; ++i) {
        FingerData f;
        f.fingerId = i;
        f.isExtended = true;
        f.setValid(true);
        f.bones.clear();
        for (int b = 0; b < 4; ++b) {
            BoneData bone;
            bone.prevJoint = {static_cast<float>(7+b), static_cast<float>(8+b), static_cast<float>(9+b)};
            bone.nextJoint = {static_cast<float>(10+b), static_cast<float>(11+b), static_cast<float>(12+b)};
            bone.setValid(true);
            f.bones.push_back(bone);
        }
        h.fingers.push_back(f);
    }
    return h;
}

TEST(DataProcessorHandPresenceTest, NoHandsPresent_SetsHandPresentFalseAndPalmCenter) {
    DeviceAliasManager aliasMgr;
    DataProcessor proc(aliasMgr, nullptr, nullptr, nullptr);
    FrameData frame;
    frame.hands.clear();
    proc.processData("test_serial", frame);
    auto state = proc.getVizState();
    EXPECT_FALSE(state.handPresent);
    EXPECT_FLOAT_EQ(state.palmNorm.x, 0.5f);
    EXPECT_FLOAT_EQ(state.palmNorm.y, 0.5f);
}

TEST(DataProcessorHandPresenceTest, OneHandPresent_SetsHandPresentTrueAndPalmNorm) {
    DeviceAliasManager aliasMgr;
    DataProcessor proc(aliasMgr, nullptr, nullptr, nullptr);
    FrameData frame;
    frame.hands.push_back(makeHand("left"));
    proc.processData("test_serial", frame);
    auto state = proc.getVizState();
    EXPECT_TRUE(state.handPresent);
    // PalmNorm should be in [0,1] range, not at center
    EXPECT_GE(state.palmNorm.x, 0.0f);
    EXPECT_LE(state.palmNorm.x, 1.0f);
    EXPECT_GE(state.palmNorm.y, 0.0f);
    EXPECT_LE(state.palmNorm.y, 1.0f);
}

TEST(DataProcessorHandPresenceTest, MultipleHandsPresent_SetsHandPresentTrue) {
    DeviceAliasManager aliasMgr;
    DataProcessor proc(aliasMgr, nullptr, nullptr, nullptr);
    FrameData frame;
    frame.hands.push_back(makeHand("left"));
    frame.hands.push_back(makeHand("right"));
    proc.processData("test_serial", frame);
    auto state = proc.getVizState();
    EXPECT_TRUE(state.handPresent);
}

TEST(DataProcessorHandPresenceTest, HandAppearsAndDisappears) {
    DeviceAliasManager aliasMgr;
    DataProcessor proc(aliasMgr, nullptr, nullptr, nullptr);
    FrameData frame;
    // Frame 1: No hands
    frame.hands.clear();
    proc.processData("test_serial", frame);
    EXPECT_FALSE(proc.getVizState().handPresent);
    // Frame 2: One hand
    frame.hands.push_back(makeHand("left"));
    proc.processData("test_serial", frame);
    EXPECT_TRUE(proc.getVizState().handPresent);
    // Frame 3: No hands again
    frame.hands.clear();
    proc.processData("test_serial", frame);
    EXPECT_FALSE(proc.getVizState().handPresent);
    EXPECT_FLOAT_EQ(proc.getVizState().palmNorm.x, 0.5f);
    EXPECT_FLOAT_EQ(proc.getVizState().palmNorm.y, 0.5f);
}
