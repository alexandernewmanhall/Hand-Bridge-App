#pragma once

// ImGuiWrapper.h - A header to wrap ImGui includes and suppress warnings

// First include our safety utilities
#include "ImGuiSafety.h"

// Disable specific warnings for ImGui code
#pragma warning(push)
#pragma warning(disable: 26495)  // Variable is uninitialized
#pragma warning(disable: 33010)  // Unchecked lower bound for enum source used as index

// Include ImGui headers
// #include "imgui.h" // REMOVED - Already included via ImGuiSafety.h
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// Restore warning settings
#pragma warning(pop) 