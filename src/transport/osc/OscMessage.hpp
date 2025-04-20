#pragma once
#include <string>
#include <vector>

struct OscMessage {
    std::string address;
    std::vector<float> values; // Extend with variant if needed
};
