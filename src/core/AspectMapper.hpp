#pragma once

// Minimal stub for AspectPreset and AspectMapper to allow test and event code to build.
struct AspectPreset {
    int width;
    int height;
    bool isLandscape;
};

class AspectMapper {
public:
    AspectMapper() = default;
    void setPreset(const AspectPreset& preset) { preset_ = preset; }
    AspectPreset getPreset() const { return preset_; }
private:
    AspectPreset preset_{16, 9, true};
};
