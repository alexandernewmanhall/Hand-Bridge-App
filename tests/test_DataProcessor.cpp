#include <gtest/gtest.h>
#include "../src/pipeline/03_DataProcessor.hpp"
#include "../src/core/FrameData.hpp"
#include "../src/core/DeviceAliasManager.hpp"
#include "../src/core/AppLogger.hpp"
// #include "core/AspectMapper.hpp"
#include <string>
#include <vector>

// Utility to create a fully valid hand for testing
HandData makeHand(const std::string& side) {
    std::cout << "[DEBUG] makeHand called for side=" << side << std::endl << std::flush;
    HandData h;
    h.handType = side;
    h.palm.position = {1.0f, 2.0f, 3.0f};
    h.valid = true;           // Ensure hand is valid
    h.arm.wristPosition = {4.0f, 5.0f, 6.0f};
    h.arm.valid = true;       // Ensure arm is valid
    h.fingers.clear();
    for (int i = 0; i < 5; ++i) {
        FingerData f;
        f.fingerId = i;
        f.isExtended = true;
        f.valid = true;       // Ensure finger is valid
        f.bones.clear();
        for (int b = 0; b < 4; ++b) {
            BoneData bone;
            bone.prevJoint = {static_cast<float>(7+b), static_cast<float>(8+b), static_cast<float>(9+b)};
            bone.nextJoint = {static_cast<float>(10+b), static_cast<float>(11+b), static_cast<float>(12+b)};
            bone.valid = true; // Ensure bone is valid
            f.bones.push_back(bone);
        }
        h.fingers.push_back(f);
    }
    return h;
}

// TEST(DataProcessorConfigTest, SetAndGetAspectPreset) { /* removed */ }

// TEST(DataProcessorConfigTest, SetAndGetWorkspaceExtents) { /* removed */ }

// TEST(DataProcessorConfigTest, SetAndGetGainParams) { /* removed */ }

// TEST(DataProcessorConfigTest, SetAndGetSoftZone) { /* removed */ }

TEST(DataProcessorConfigTest, MappingAffectOSC) {
    DeviceAliasManager aliasMgr;
    std::vector<std::string> oscAddresses;
    DataProcessor proc(aliasMgr, [&](const OscMessage& msg) {
        std::cout << "[DEBUG][OSC CALLBACK] Received OSC address: " << msg.address << std::endl << std::flush;
        oscAddresses.push_back(msg.address);
    }, [](const FrameData&) {}, nullptr);
    proc.setFilterSettings(true, true, true, true, true, true, true, false, false, false, false, false, false, false); // Enable all fingers, palm, wrist
    std::cout << "[DEBUG][TEST] Filter flags: sendPalm_=" << proc.sendPalm_
              << ", sendWrist_=" << proc.sendWrist_
              << ", sendThumb_=" << proc.sendThumb_
              << ", sendIndex_=" << proc.sendIndex_
              << ", sendMiddle_=" << proc.sendMiddle_
              << ", sendRing_=" << proc.sendRing_
              << ", sendPinky_=" << proc.sendPinky_ << std::endl << std::flush;
    FrameData frame;
    frame.deviceId = "serialA";
    HandData hand = makeHand("left");
    hand.palm.position = {0, 450, 0}; // Center
    hand.setValid(true); // Ensure hand is valid
    frame.hands.push_back(hand);
    std::cout << "[DEBUG][TEST] frame.hands.size()=" << frame.hands.size() << std::endl << std::flush;
    if (!frame.hands.empty()) {
        const auto& hand = frame.hands[0];
        std::cout << "[DEBUG][TEST] handType=" << hand.handType
                  << ", hand.valid=" << hand.valid
                  << ", hand.isValid()=" << hand.isValid()
                  << ", arm.valid=" << hand.arm.valid
                  << ", arm.isValid()=" << hand.arm.isValid()
                  << ", fingers.size()=" << hand.fingers.size() << std::endl << std::flush;
    }
    std::string alias = aliasMgr.getOrAssignAlias("serialA");
    std::cout << "[DEBUG][TEST] alias=" << alias << std::endl << std::flush;
    proc.processData("serialA", frame);
    std::string alias2 = aliasMgr.getOrAssignAlias("serialA");
    std::vector<std::string> expected = {
        "/leap/" + alias2 + "/left/palm/tx",
        "/leap/" + alias2 + "/left/palm/ty",
        "/leap/" + alias2 + "/left/palm/tz",
        "/leap/" + alias2 + "/left/wrist/tx",
        "/leap/" + alias2 + "/left/wrist/ty",
        "/leap/" + alias2 + "/left/wrist/tz",
        "/leap/" + alias2 + "/left/finger/thumb/exists",
        "/leap/" + alias2 + "/left/finger/index/exists",
        "/leap/" + alias2 + "/left/finger/middle/exists",
        "/leap/" + alias2 + "/left/finger/ring/exists",
        "/leap/" + alias2 + "/left/finger/pinky/exists"
        "/leap/" + alias + "/left/palm/tz",
        "/leap/" + alias + "/left/wrist/tx",
        "/leap/" + alias + "/left/wrist/ty",
        "/leap/" + alias + "/left/wrist/tz",
        "/leap/" + alias + "/left/finger/thumb/exists",
        "/leap/" + alias + "/left/finger/index/exists",
        "/leap/" + alias + "/left/finger/middle/exists",
        "/leap/" + alias + "/left/finger/ring/exists",
        "/leap/" + alias + "/left/finger/pinky/exists"
    };
    std::cout << "[TEST] All OSC addresses (MappingAffectOSC):" << std::endl << std::flush;
    for (const auto& addr : oscAddresses) {
        std::cout << "[DEBUG] Actual OSC address: " << addr << std::endl << std::flush;
    }
    if (oscAddresses.empty()) {
        std::cout << "[ERROR] No OSC addresses were emitted!" << std::endl << std::flush;
    }
    std::cout << "[DEBUG] Expected OSC addresses:" << std::endl << std::flush;
    for (const auto& exp : expected) {
        std::cout << exp << std::endl << std::flush;
    }
    EXPECT_FALSE(oscAddresses.empty());
    for (const auto& exp : expected) {
        bool found = false;
        for (const auto& addr : oscAddresses) {
            if (addr.find(exp) != std::string::npos) found = true;
        }
        EXPECT_TRUE(found);
    }
}

TEST(DataProcessorTest, SendsPalmAndWristMessages) {
    DeviceAliasManager aliasMgr;
    std::vector<std::string> oscAddresses;
    DataProcessor proc(aliasMgr, [&](const OscMessage& msg) {
        std::cout << "[DEBUG][OSC CALLBACK] Received OSC address: " << msg.address << std::endl << std::flush;
        oscAddresses.push_back(msg.address);
    }, [](const FrameData&) {}, nullptr);
    // Only palm and wrist
    proc.setFilterSettings(
        true,  // palm
        true,  // wrist
        false,  // thumb
        false,  // index
        false,  // middle
        false,  // ring
        false,  // pinky
        false,  // palm orientation
        false,  // palm velocity
        false,  // palm normal
        false,  // visible time
        false,  // is extended
        false,  // pinch
        false   // grab
    );
    FrameData frame;
    frame.deviceId = "serialA";
    frame.hands.push_back(makeHand("left"));
    std::cout << "[TEST] SendsPalmAndWristMessages: hand type: " << frame.hands[0].handType << ", fingers size: " << frame.hands[0].fingers.size() << std::endl << std::flush;
    for (size_t i = 0; i < frame.hands[0].fingers.size(); ++i) {
        std::cout << "[DEBUG] Finger " << i << ": valid=" << frame.hands[0].fingers[i].isValid()
                  << " bones=" << frame.hands[0].fingers[i].bones.size() << std::endl << std::flush;
    }
    proc.processData("serialA", frame);
    // Print all OSC addresses for debugging
    std::cout << "[TEST] All OSC addresses:" << std::endl << std::flush;
    for (const auto& addr : oscAddresses) {
        std::cout << addr << std::endl << std::flush;
    }
    // Should have palm and wrist OSC addresses
    std::string alias = aliasMgr.getOrAssignAlias("serialA");
    std::vector<std::string> expectedPalmWrist = {
        "/leap/" + alias + "/left/palm/tx",
        "/leap/" + alias + "/left/palm/ty",
        "/leap/" + alias + "/left/palm/tz",
        "/leap/" + alias + "/left/wrist/tx",
        "/leap/" + alias + "/left/wrist/ty",
        "/leap/" + alias + "/left/wrist/tz"
    };
    if (oscAddresses.empty()) {
        std::cout << "[ERROR] No OSC addresses were emitted!" << std::endl << std::flush;
    }
    std::cout << "[DEBUG] Expected OSC addresses:" << std::endl << std::flush;
    for (const auto& exp : expectedPalmWrist) {
        std::cout << exp << std::endl << std::flush;
    }
    std::cout << "[DEBUG] Actual OSC addresses:" << std::endl << std::flush;
    for (const auto& act : oscAddresses) {
        std::cout << act << std::endl << std::flush;
    }
    for (const auto& exp : expectedPalmWrist) {
        bool found = false;
        for (const auto& addr : oscAddresses) {
            if (addr.find(exp) != std::string::npos) found = true;
        }
        EXPECT_TRUE(found);
    }
}

TEST(DataProcessorTest, FingerFiltersWork) {
    DeviceAliasManager aliasMgr;
    std::vector<std::string> oscAddresses;
    auto logger2 = std::make_shared<AppLogger>();
    DataProcessor proc(aliasMgr, [&](const OscMessage& msg) {
        std::cout << "Callback called: " << msg.address << std::endl << std::flush;
        std::cout << "[DEBUG][OSC CALLBACK] Received OSC address: " << msg.address << std::endl << std::flush;
        oscAddresses.push_back(msg.address);
    }, [](const FrameData&) {}, logger2);
    // Only thumb
    proc.setFilterSettings(
        false,  // palm
        false,  // wrist
        true,   // thumb
        false,  // index
        false,  // middle
        false,  // ring
        false,  // pinky
        false,  // palm orientation
        false,  // palm velocity
        false,  // palm normal
        false,  // visible time
        false,  // is extended
        false,  // pinch
        false   // grab
    );
    FrameData frame;
    frame.deviceId = "serialA";
    frame.hands.push_back(makeHand("right"));
    std::cout << "[TEST] FingerFiltersWork: hand type: " << frame.hands[0].handType << ", fingers size: " << frame.hands[0].fingers.size() << std::endl << std::flush;
    for (size_t i = 0; i < frame.hands[0].fingers.size(); ++i) {
        std::cout << "[DEBUG] Finger " << i << ": valid=" << frame.hands[0].fingers[i].isValid()
                  << " bones=" << frame.hands[0].fingers[i].bones.size() << std::endl << std::flush;
    }
    proc.processData("serialA", frame);
    // Print all OSC addresses for debugging
    std::cout << "[TEST] All OSC addresses (FingerFiltersWork):" << std::endl << std::flush;
    for (const auto& addr : oscAddresses) {
        std::cout << addr << std::endl << std::flush;
    }
    // Should have thumb OSC address (special case: /thumb/exists)
    std::string alias = aliasMgr.getOrAssignAlias("serialA");
    std::string expectedThumb = "/leap/" + alias + "/right/finger/thumb/exists";
    std::cout << "[DEBUG] Looking for expected OSC address: " << expectedThumb << std::endl << std::flush;
    bool found = false;
    for (const auto& addr : oscAddresses) {
        std::cout << "[DEBUG] Actual OSC address: " << addr << std::endl << std::flush;
        if (addr.find(expectedThumb) != std::string::npos) found = true;
    }
    if (!found) {
        std::cout << "[ERROR] Expected OSC address not found: " << expectedThumb << std::endl << std::flush;
    }
    EXPECT_TRUE(found);
    // Should NOT have palm or wrist
    bool foundPalm = false, foundWrist = false;
    for (const auto& addr : oscAddresses) {
        if (addr.find("/leap/" + alias + "/right/palm/") != std::string::npos) foundPalm = true;
        if (addr.find("/leap/" + alias + "/right/wrist/") != std::string::npos) foundWrist = true;
    }
    EXPECT_FALSE(foundPalm);
    EXPECT_FALSE(foundWrist);
}

TEST(DataProcessorTest, MinimalTest) {
    DeviceAliasManager aliasMgr;
    std::vector<std::string> oscAddresses;
    FrameData frame;
    frame.deviceId = "serialA";
    frame.hands.push_back(makeHand("right"));
    std::cout << "[TEST] MinimalTest: hand type: " << frame.hands[0].handType << ", fingers size: " << frame.hands[0].fingers.size() << std::endl << std::flush;
    for (size_t i = 0; i < frame.hands[0].fingers.size(); ++i) {
        std::cout << "[DEBUG] Finger " << i << ": valid=" << frame.hands[0].fingers[i].isValid()
                  << " bones=" << frame.hands[0].fingers[i].bones.size() << std::endl << std::flush;
    }
    ASSERT_EQ(frame.hands[0].fingers.size(), 5);
    bool callbackCalled = false;
    auto logger3 = std::make_shared<AppLogger>();
    DataProcessor proc2(aliasMgr, [&](const OscMessage& msg) {
        std::cout << "[TEST] Callback called: " << msg.address << std::endl << std::flush;
        callbackCalled = true;
        std::cout << "[DEBUG][OSC CALLBACK] Received OSC address: " << msg.address << std::endl << std::flush;
        oscAddresses.push_back(msg.address);
    }, [](const FrameData&) {}, logger3);
    // Only thumb enabled
    proc2.setFilterSettings(
        false,  // palm
        false,  // wrist
        true,   // thumb
        false,  // index
        false,  // middle
        false,  // ring
        false,  // pinky
        false,  // palm orientation
        false,  // palm velocity
        false,  // palm normal
        false,  // visible time
        false,  // is extended
        false,  // pinch
        false   // grab
    );
    proc2.processData("serialA", frame);
    // Print all OSC addresses for debugging
    std::cout << "[TEST] All OSC addresses (MinimalTest):" << std::endl << std::flush;
    for (const auto& addr : oscAddresses) {
        std::cout << "[DEBUG] Actual OSC address: " << addr << std::endl << std::flush;
    }
    bool found = false;
    std::string expectedThumb = "/leap/" + aliasMgr.getOrAssignAlias("serialA") + "/right/finger/thumb/exists";
    for (const auto& addr : oscAddresses) {
        if (addr.find(expectedThumb) != std::string::npos) found = true;
    }
    if (!callbackCalled) {
        std::cout << "[ERROR] Callback was never called!" << std::endl << std::flush;
    }
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(found);
}

TEST(DataProcessorTest, NoMessagesIfNothingEnabled) {
    DeviceAliasManager aliasMgr;
    std::vector<std::string> oscAddresses;
    DataProcessor proc(aliasMgr, [&](const OscMessage& msg) {
        std::cout << "[DEBUG][OSC CALLBACK] Received OSC address: " << msg.address << std::endl << std::flush;
        oscAddresses.push_back(msg.address);
    }, [](const FrameData&) {}, nullptr);
    proc.setFilterSettings(false, false, false, false, false, false, false, false, false, false, false, false, false, false); // All off

    FrameData frame;
    frame.deviceId = "serialA";
    frame.hands.push_back(makeHand("left"));

    proc.processData("serialA", frame);
    EXPECT_TRUE(oscAddresses.empty());
}

// --- Test Fixture --- (using MockConfigManager)
class DataProcessorTest : public ::testing::Test {
protected:
    MockConfigManager mockConfigManager;
    DeviceAliasManager aliasManager;
    DataProcessor processor;

    DataProcessorTest() : processor(mockConfigManager, aliasManager) {}

    void SetUp() override {
        // Set default expectations for config manager calls if needed
        EXPECT_CALL(mockConfigManager, getInt("osc_port", ::testing::_))
            .WillRepeatedly(::testing::Return(9001));
        EXPECT_CALL(mockConfigManager, getString("osc_address", ::testing::_))
            .WillRepeatedly(::testing::Return("127.0.0.1"));
        EXPECT_CALL(mockConfigManager, getBool("osc_send_raw", ::testing::_))
            .WillRepeatedly(::testing::Return(true));
        // Add expectations for gain/softzone if they were read in constructor
        // EXPECT_CALL(mockConfigManager, getFloat("gain_x", ::testing::_)).WillRepeatedly(::testing::Return(1.0f));
        // EXPECT_CALL(mockConfigManager, getFloat("soft_zone_width", ::testing::_)).WillRepeatedly(::testing::Return(0.1f));
        
        // Initialize OSC sender for the processor
        // (Use a real or mock OscSender depending on test needs)
        // For now, assume DataProcessor doesn't strictly need a sender for logic tests
    }
};

// --- Tests for removed features (Commented Out) ---
/*
TEST_F(DataProcessorTest, ApplySymmetricElasticEdges) {
    // Test cases for applySymmetricElasticEdges
    processor.setSoftZone(0.1f); // Example soft zone
    EXPECT_FLOAT_EQ(processor.applySymmetricElasticEdges(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(processor.applySymmetricElasticEdges(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(processor.applySymmetricElasticEdges(-1.0f), -1.0f);
    // Test edge cases near 1.0 and -1.0 based on soft zone
    EXPECT_GT(processor.applySymmetricElasticEdges(0.95f), 0.95f); // Should be pushed out
    EXPECT_LT(processor.applySymmetricElasticEdges(-0.95f), -0.95f);
    // Test values outside [-1, 1] - behavior might depend on implementation
    // EXPECT_FLOAT_EQ(processor.applySymmetricElasticEdges(1.1f), ???);
}

TEST_F(DataProcessorTest, ComputeGain) {
    // Test cases for computeGain
    processor.setGainParams(1.0f, 3.0f, 6.0f, 80.0f, 240.0f); // Example gain params
    EXPECT_FLOAT_EQ(processor.computeGain(0.0f), 1.0f);    // Low speed
    EXPECT_FLOAT_EQ(processor.computeGain(79.0f), 1.0f);  // Near low threshold
    EXPECT_GT(processor.computeGain(81.0f), 1.0f);      // Above low threshold
    EXPECT_FLOAT_EQ(processor.computeGain(160.0f), 3.0f); // Mid speed
    EXPECT_GT(processor.computeGain(241.0f), 3.0f);   // Above mid threshold
    EXPECT_FLOAT_EQ(processor.computeGain(500.0f), 6.0f); // High speed (max gain)
}

TEST_F(DataProcessorTest, SetGainParams) {
    // Verify setting gain parameters works
    processor.setGainParams(2.0f, 4.0f, 8.0f, 100.0f, 300.0f);
    // Maybe add internal state check or check computed gain with new params
    EXPECT_FLOAT_EQ(processor.computeGain(50.0f), 2.0f);
}

TEST_F(DataProcessorTest, SetSoftZone) {
    // Verify setting soft zone works
    processor.setSoftZone(0.2f);
    // Maybe add internal state check or check edge application with new zone
    EXPECT_GT(processor.applySymmetricElasticEdges(0.85f), 0.85f);
}
*/

// --- Test Hand Data Processing (Example) ---
TEST_F(DataProcessorTest, ProcessSingleHandData) {
    // Create mock HandData
    HandData hand;
    hand.isValid = true;
    hand.isLeft = true;
    // ... populate other hand data fields (palm, wrist, fingers) ...
    hand.palmPosition = glm::vec3(0.1f, 0.2f, 0.3f);
    hand.wristPosition = glm::vec3(0.0f, 0.1f, 0.2f);
    hand.pinchStrength = 0.7f;
    hand.grabStrength = 0.3f;
    // ... (add finger data)

    // Create mock FrameData
    FrameData frame;
    frame.hands.push_back(hand);
    frame.timestamp = 12345;
    frame.frameId = 1;
    frame.frameRate = 120.0f;
    frame.interactionBox = { glm::vec3(-150, 0, -150), glm::vec3(150, 300, 150) };

    // Mock OSC Sender (if strict testing is needed)
    MockOscSender mockSender;
    processor.setOscSender(&mockSender);

    // Expect OSC messages based on the hand data and default filters
    // Example (adjust based on actual OSC messages sent):
    EXPECT_CALL(mockSender, sendOscMessage("device0", "l", "/leap/hand/palm", "fff", ::testing::_, ::testing::_, ::testing::_));
    // EXPECT_CALL(mockSender, sendOscMessage("device0", "l", "/leap/hand/wrist", ...)); // Wrist might be off by default
    EXPECT_CALL(mockSender, sendOscMessage("device0", "l", "/leap/hand/pinch", "f", hand.pinchStrength));
    EXPECT_CALL(mockSender, sendOscMessage("device0", "l", "/leap/hand/grab", "f", hand.grabStrength));
    // ... add expectations for enabled fingers ...

    // Process the data
    processor.processData("TestSerial", frame);

    // Assertions on processor state (if any)
    // EXPECT_EQ(processor.getSomeInternalState(), expectedValue);
}

// Add more tests for multiple hands, different filters, etc.

// --- Main function for running tests ---
/* // Don't need a separate main if using gtest_main
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/
