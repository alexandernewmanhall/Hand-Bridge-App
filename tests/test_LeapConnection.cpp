#include <gtest/gtest.h>
#include "../src/pipeline/00_LeapConnection.hpp"
#include <iostream>

// Smoke test for LeapConnection: only runs if Leap Motion service is available
TEST(LeapConnectionTest, CanCreateAndDestroy) {
    try {
        LeapConnection conn;
        EXPECT_NE(conn.getConnection(), nullptr) << "Connection handle should not be null.";
        std::cout << "LeapConnection created and destroyed successfully." << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Exception in LeapConnection: " << ex.what() << std::endl;
        FAIL() << "LeapConnection threw exception: " << ex.what();
    }
}

// Note: Error/exception cases (e.g., LeapCreateConnection failure) require
// mocking the LeapC API, which is not set up here. For now, this test only
// verifies successful creation/destruction if the Leap Motion runtime is present.
