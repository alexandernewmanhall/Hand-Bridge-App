#pragma once
#include <iostream>

// Toggle this define to enable/disable logging globally
#define ENABLE_LOGGING 1

#if ENABLE_LOGGING
    #define LOG(msg) (std::cout << msg << std::endl)
    #define LOG_ERR(msg) (std::cerr << msg << std::endl)
#else
    #define LOG(msg) ((void)0)
    #define LOG_ERR(msg) ((void)0)
#endif
