#pragma once

// Basic vector and quaternion types needed for Leap Motion tracking data
struct Vector3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Vector3f() = default;
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Quaternion4f {
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Quaternion4f() = default;
    Quaternion4f(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}
}; 