#pragma once

#include <utility> // For std::exchange
#include <functional> // If we need std::function for complex deleters later

/**
 * @brief A generic RAII wrapper for C-style handles.
 *
 * Manages a handle of type T, automatically calling the provided Deleter
 * function when the Handle object goes out of scope.
 *
 * @tparam T The type of the handle (e.g., LEAP_CONNECTION, HICON, HANDLE).
 * @tparam Deleter A function pointer or functor type used to release/delete the handle.
 *         It must be callable with an argument of type T.
 */
template<typename T, auto Deleter>
class Handle {
public:
    // Default constructor: creates a null handle.
    Handle() = default;

    // Explicit constructor: takes ownership of an existing handle.
    explicit Handle(T h) : h_(h) {}

    // Destructor: releases the handle if it's valid.
    ~Handle() {
        if (h_) {
            Deleter(h_);
        }
    }

    // Delete copy constructor and copy assignment operator.
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    // Move constructor: transfers ownership from another Handle.
    Handle(Handle&& other) noexcept : h_(std::exchange(other.h_, T{})) {} // Use T{} for generic null

    // Move assignment operator: transfers ownership from another Handle.
    Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            // Release the current handle, if any
            if (h_) {
                Deleter(h_);
            }
            // Transfer ownership from the other handle
            h_ = std::exchange(other.h_, T{}); // Use T{} for generic null
        }
        return *this;
    }

    // Gets the underlying raw handle. Use with caution.
    T get() const { return h_; }

    // Releases ownership of the handle and returns it. The caller is now responsible.
    T release() {
        return std::exchange(h_, T{}); // Use T{} for generic null
    }

    // Resets the handle, taking ownership of a new one and releasing the old one.
    void reset(T h = T{}) { // Use T{} for generic null
        if (h_ != h) {
            if (h_) {
                Deleter(h_);
            }
            h_ = h;
        }
    }

    // Checks if the handle is valid (not null).
    explicit operator bool() const {
        // Add specific checks if T{} isn't the only invalid state
        return h_ != T{};
    }

private:
    T h_ = T{}; // Initialize with a generic null value for the handle type T
};