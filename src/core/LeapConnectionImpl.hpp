#pragma once

#include "core/interfaces/ILeapConnection.hpp"
#include "LeapC.h"
#include <memory>
#include <stdexcept>
#include "utils/HandleWrapper.hpp"

// Deleter function for Leap Connection
inline void LeapDestroyConnectionWrapper(LEAP_CONNECTION conn) {
    if (conn) LeapDestroyConnection(conn);
}

// Define the specific handle type
using LeapConnectionHandle = Handle<LEAP_CONNECTION, LeapDestroyConnectionWrapper>;

// Concrete implementation of ILeapConnection using LeapC
class LeapConnectionImpl : public ILeapConnection {
public:
    LeapConnectionImpl();
    ~LeapConnectionImpl() override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    LEAP_CONNECTION getRawConnection() const;
private:
    LeapConnectionHandle connectionHandle_;
    bool connected_ = false;
};
