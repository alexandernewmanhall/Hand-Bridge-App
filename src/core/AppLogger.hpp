#pragma once

#include <string>
#include <windows.h> // For OutputDebugStringA

// Simple wrapper class for logging via OutputDebugStringA
// Allows the logger to be managed as a service via shared_ptr
class AppLogger {
public:
    virtual ~AppLogger() = default; // Virtual destructor for potential inheritance

    virtual void log(const std::string& message) const {
        std::string logMsg = message + "\n"; // Add newline for readability in debugger
        OutputDebugStringA(logMsg.c_str());
    }
};