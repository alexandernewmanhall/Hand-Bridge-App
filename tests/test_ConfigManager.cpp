#include <gtest/gtest.h>
#include "../src/core/ConfigManager.h"
#include <fstream>
#include <cstdio>

TEST(ConfigManagerTest, SaveAndLoadConfig) {
    ConfigManager config;
    config.setOscIp("192.168.1.42");
    config.setOscPort(9000);
    config.setLowLatencyMode(true);
    config.setDefaultHandAssignment("serialA", "left");
    config.setDefaultHandAssignment("serialB", "right");
    config.getDeviceAliasManager().getOrAssignAlias("serialA");
    config.getDeviceAliasManager().getOrAssignAlias("serialB");

    // Save to temp file
    std::string filename = "test_config.json";
    ASSERT_TRUE(config.saveConfig(filename));
    
    // Make a new config and load
    ConfigManager loaded;
    ASSERT_TRUE(loaded.loadConfig(filename));
    EXPECT_EQ(loaded.getOscIp(), "192.168.1.42");
    EXPECT_EQ(loaded.getOscPort(), 9000);
    EXPECT_TRUE(loaded.getLowLatencyMode());
    EXPECT_EQ(loaded.getDefaultHandAssignment("serialA"), "left");
    EXPECT_EQ(loaded.getDefaultHandAssignment("serialB"), "right");
    // Aliases should persist
    EXPECT_EQ(loaded.getDeviceAliasManager().getOrAssignAlias("serialA"), config.getDeviceAliasManager().getOrAssignAlias("serialA"));
    EXPECT_EQ(loaded.getDeviceAliasManager().getOrAssignAlias("serialB"), config.getDeviceAliasManager().getOrAssignAlias("serialB"));

    // Cleanup
    std::remove(filename.c_str());
}

TEST(ConfigManagerTest, HandlesMissingFile) {
    ConfigManager config;
    // Should return false for missing file
    ASSERT_FALSE(config.loadConfig("nonexistent_file.json"));
}
