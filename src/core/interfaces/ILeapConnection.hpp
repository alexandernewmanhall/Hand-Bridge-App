#pragma once

// Minimal interface for Leap connection management
class ILeapConnection {
public:
    virtual ~ILeapConnection() = default;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    // Add more as needed, e.g. polling frames
};
