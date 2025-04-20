#pragma once

#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

/**
 * Utility functions for managing thread affinity.
 * 
 * These functions allow pinning threads to specific CPU cores to reduce
 * context switching and improve cache locality.
 */
class ThreadAffinity {
public:
    /**
     * Sets thread affinity to a specific core.
     * 
     * @param thread The thread to set affinity for
     * @param coreIndex The zero-based index of the core to pin the thread to
     * @return True if successful, false otherwise
     */
    static bool pinThreadToCore(std::thread& thread, int coreIndex) {
#ifdef _WIN32
        // Windows implementation
        DWORD_PTR mask = 1ULL << coreIndex;
        DWORD_PTR result = SetThreadAffinityMask(thread.native_handle(), mask);
        return result != 0;
#else
        // Linux/Unix implementation
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(coreIndex, &cpuSet);
        int result = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuSet);
        return result == 0;
#endif
    }

    /**
     * Gets the number of available CPU cores.
     * 
     * @return The number of CPU cores
     */
    static int getProcessorCount() {
        return std::thread::hardware_concurrency();
    }
}; 