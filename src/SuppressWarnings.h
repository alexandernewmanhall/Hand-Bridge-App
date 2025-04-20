/**
 * SuppressWarnings.h
 * Contains pragmas and includes to suppress warnings from external libraries
 */

#pragma once

// Disable warning C26819: Unannotated fallthrough between switch labels
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 26819)
#endif

// Include external headers that trigger warnings here
#include <SDL_stdinc.h>

// For ImGui warning suppression - add relevant paths as needed
#ifdef __has_include
  #if __has_include("imstb_truetype.h")
    #include "imstb_truetype.h"
  #elif __has_include("../garbage/imgui/imstb_truetype.h")
    #include "../garbage/imgui/imstb_truetype.h"
  #elif __has_include("../../external/imgui/imstb_truetype.h")
    #include "../../external/imgui/imstb_truetype.h"
  #endif
#endif

// Re-enable warnings
#ifdef _MSC_VER
    #pragma warning(pop)
#endif 