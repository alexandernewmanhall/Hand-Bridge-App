#pragma once
#include <string> // For std::string
#include <cstddef> // For size_t

// Forward declare or include OscMessage definition
#include "transport/osc/OscMessage.hpp"

// Abstract interface for message transport sinks (OSC, TCP, etc.)
class ITransportSink {
public:
    virtual ~ITransportSink() = default;
    virtual bool send(const void* data, size_t size) = 0;
    virtual void sendOscMessage(const OscMessage& message) = 0;
    virtual void updateTarget(const std::string& target, int port) = 0;
    virtual void close() = 0;
    // Add more as needed for transport
};
