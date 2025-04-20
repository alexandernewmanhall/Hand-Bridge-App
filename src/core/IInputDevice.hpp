#pragma once

class IInputDevice {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~IInputDevice() = default;
};
