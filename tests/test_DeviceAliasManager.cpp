#include <gtest/gtest.h>
#include "../src/core/DeviceAliasManager.hpp"

TEST(DeviceAliasManagerTest, AssignsUniqueAliases) {
    DeviceAliasManager manager;
    std::string alias1 = manager.getOrAssignAlias("serial123");
    std::string alias2 = manager.getOrAssignAlias("serial456");
    EXPECT_NE(alias1, alias2);
}

TEST(DeviceAliasManagerTest, ReturnsSameAliasForSameSerial) {
    DeviceAliasManager manager;
    std::string alias1 = manager.getOrAssignAlias("serialABC");
    std::string alias2 = manager.getOrAssignAlias("serialABC");
    EXPECT_EQ(alias1, alias2);
}

TEST(DeviceAliasManagerTest, ClearsAliases) {
    DeviceAliasManager manager;
    std::string alias1 = manager.getOrAssignAlias("serialXYZ");
    manager.clear();
    std::string alias2 = manager.getOrAssignAlias("serialXYZ");
    EXPECT_EQ(alias1, alias2); // After clear, same serial should get the same alias
}
