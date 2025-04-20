#pragma once

#include <vector>
#include <atomic>
#include <optional>
#include <stdexcept> // For std::runtime_error
#include <new>       // For std::hardware_destructive_interference_size

// Simple lock-free Single-Producer Single-Consumer (SPSC) queue using a ring buffer.
// Stores items by value (copy or move). Not suitable for types that cannot be trivially copied/moved.
// Fixed capacity, designed for FrameData or similar structures.
template<typename T>
class SpscQueue {
    // Prevent false sharing by padding cache lines
    static constexpr size_t cache_line_size = std::hardware_destructive_interference_size;

    alignas(cache_line_size) std::atomic<size_t> head_{0};
    alignas(cache_line_size) std::atomic<size_t> tail_{0};
    // Ensure buffer starts on a new cache line if needed, though vector allocation might handle this.
    alignas(cache_line_size) std::vector<T> buffer_;
    const size_t capacity_; // Actual capacity is capacity_ - 1

public:
    explicit SpscQueue(size_t capacity) : capacity_(capacity + 1) // Need one extra slot for empty/full check
    {
        if (capacity < 1) {
            throw std::invalid_argument("SpscQueue capacity must be at least 1");
        }
        buffer_.resize(capacity_ + 1); // Allocate buffer space (+1 for the unused slot)
    }

    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    // Attempts to push an item into the queue (producer only).
    // Uses move semantics if possible.
    // Returns true if successful, false if the queue is full.
    bool try_push(T&& item) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) % capacity_;

        // Check if queue is full (tail + 1 == head)
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }

        // Move the item into the buffer slot
        buffer_[current_tail] = std::move(item);

        // Make the item available to the consumer
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

     // Attempts to push an item into the queue (producer only).
     // Uses copy semantics.
     // Returns true if successful, false if the queue is full.
    bool try_push(const T& item) noexcept {
        T temp = item; // Create a temporary copy
        return try_push(std::move(temp)); // Use move push
    }

    // Attempts to pop an item from the queue (consumer only).
    // Returns an std::optional containing the item if successful,
    // or std::nullopt if the queue is empty.
    std::optional<T> try_pop() noexcept {
        const size_t current_head = head_.load(std::memory_order_relaxed);

        // Check if queue is empty (head == tail)
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt; // Queue is empty
        }

        // Retrieve the item (move it out)
        T item = std::move(buffer_[current_head]);

        // Move head forward, making the slot available
        const size_t next_head = (current_head + 1) % capacity_;
        head_.store(next_head, std::memory_order_release);

        return item; // Return the retrieved item by value (moved)
    }

    // Checks if the queue is empty (consumer perspective).
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    // Note: size() is approximate and mainly for debugging/metrics,
    // as head/tail can change concurrently.
    size_t size_approx() const noexcept {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail >= head) {
            return tail - head;
        } else {
            return capacity_ + tail - head;
        }
    }
};