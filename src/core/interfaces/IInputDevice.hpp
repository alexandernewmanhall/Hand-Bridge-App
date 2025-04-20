#pragma once

// Abstract interface for any input device (Leap, other sensors)
class IInputDevice {
public:
    virtual ~IInputDevice() = default;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    // Add more as needed for device input
};
