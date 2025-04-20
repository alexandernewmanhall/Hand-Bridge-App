#pragma once
#include <iostream>

// Logging macros for development/debugging only.
// In Release builds, logging is disabled by default unless ENABLE_LOGGING is explicitly set.
// To enable logging in Release, define ENABLE_LOGGING as 1 before including this header.
#ifndef ENABLE_LOGGING
    #ifdef NDEBUG
        #define ENABLE_LOGGING 0
    #else
        #define ENABLE_LOGGING 1
    #endif
#endif

#if ENABLE_LOGGING
    #include <iostream>
    #define LOG(msg) (std::cout << msg << std::endl)
    #define LOG_ERR(msg) (std::cerr << msg << std::endl)
#else
    #define LOG(msg) ((void)0)
    #define LOG_ERR(msg) ((void)0)
#endif
