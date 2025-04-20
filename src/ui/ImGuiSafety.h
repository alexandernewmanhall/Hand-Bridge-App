#pragma once

// ImGuiSafety.h - A header to provide safer wrappers for ImGui functions with enum boundary issues

#include "imgui.h"
#include "imgui_internal.h" // Need to include internal header for ImGuiInputSource and ImGuiMouseSource enums

namespace ImGuiSafety {

    // Safe wrapper for ImGuiInputSource enum used in array indexing
    inline const char* GetInputSourceName(ImGuiInputSource source) {
        static const char* input_source_names[] = { "None", "Mouse", "Keyboard", "Gamepad" };
        static const int input_source_count = sizeof(input_source_names) / sizeof(input_source_names[0]);
        
        // Check bounds (fixes C33010 warning)
        if (source < 0 || source >= input_source_count) {
            return "Unknown";
        }
        
        return input_source_names[source];
    }

    // Safe wrapper for ImGuiMouseSource enum used in array indexing
    inline const char* GetMouseSourceName(ImGuiMouseSource source) {
        static const char* mouse_source_names[] = { "Mouse", "TouchScreen", "Pen" };
        static const int mouse_source_count = sizeof(mouse_source_names) / sizeof(mouse_source_names[0]);
        
        // Check bounds (fixes C33010 warning)
        if (source < 0 || source >= mouse_source_count) {
            return "Unknown";
        }
        
        return mouse_source_names[source];
    }

    // Add additional safe wrappers for other enum-to-array indexing operations as needed
} 