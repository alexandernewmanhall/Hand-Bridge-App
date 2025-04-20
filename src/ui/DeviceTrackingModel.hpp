#pragma once
#include <string>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>

class DeviceTrackingModel {
public:
    struct PerDeviceTrackingData {
        std::string serialNumber;
        bool isConnected = false;
        std::string assignedHand;
        std::atomic<double> frameRate{0.0};
        std::atomic<int> handCount{0};
        uint64_t frameCount = 0;
    };

    void connectDevice(const std::string& serial) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto& data = devices_[serial];
        data.serialNumber = serial;
        data.isConnected = true;
    }

    void assignHand(const std::string& serial, const std::string& hand) {
        std::lock_guard<std::mutex> lock(mtx_);
        devices_[serial].assignedHand = hand;
    }

    void setHandCount(const std::string& serial, int count) {
        std::lock_guard<std::mutex> lock(mtx_);
        devices_[serial].handCount = count;
    }

    void setFrameCount(const std::string& serial, uint64_t count) {
        std::lock_guard<std::mutex> lock(mtx_);
        devices_[serial].frameCount = count;
    }

    PerDeviceTrackingData& getDevice(const std::string& serial) {
        std::lock_guard<std::mutex> lock(mtx_);
        return devices_[serial];
    }

    std::vector<std::string> getConnectedSerials() {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<std::string> out;
        for (const auto& kv : devices_) {
            if (kv.second.isConnected) out.push_back(kv.first);
        }
        return out;
    }

private:
    std::map<std::string, PerDeviceTrackingData> devices_;
    std::mutex mtx_;
};
