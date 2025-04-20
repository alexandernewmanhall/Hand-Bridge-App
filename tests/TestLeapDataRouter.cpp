#include "../src/core/LeapDataRouter.hpp"
#include <cassert>
#include <iostream>

void testFiltering() {
    bool callbackCalled = false;
    LeapDataRouter router([
        &callbackCalled
    ](const std::string& deviceId, const FilteredFrameData& filtered) {
        callbackCalled = true;
        assert(deviceId == "abc123");
        assert(filtered.hands.size() == 1);
        assert(filtered.hands[0].handType == "left");
    });

    router.setDeviceHand("abc123", "left");
    RawFrameData frame;
    HandData leftHand;
leftHand.handType = "left";
frame.hands.push_back(leftHand);
HandData rightHand;
rightHand.handType = "right";
frame.hands.push_back(rightHand);
    router.processFrame("abc123", frame);
    assert(callbackCalled);
}

int main() {
    testFiltering();
    std::cout << "LeapDataRouter test passed!\n";
    return 0;
}
