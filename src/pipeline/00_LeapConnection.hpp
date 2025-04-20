#pragma once
#include "LeapC.h"

// 00_LeapConnection: Manages the lifecycle of a LEAP_CONNECTION for the pipeline
class LeapConnection {
public:
    LeapConnection();
    ~LeapConnection();
    LeapConnection(const LeapConnection&) = delete;
    LeapConnection& operator=(const LeapConnection&) = delete;
    LEAP_CONNECTION getConnection() const;
private:
    LEAP_CONNECTION connection_;
};
