cmake_minimum_required(VERSION 3.10)
project(leapMotionApp LANGUAGES C CXX)

# Set Leap Motion SDK Path (Adjust this path to your Leap SDK installation)
set(LEAP_SDK_PATH "C:/Program Files/Ultraleap/LeapSDK" CACHE PATH "Path to Leap Motion SDK")
message(STATUS "Using Leap SDK from: ${LEAP_SDK_PATH}")

# Define include and lib paths based on SDK Path
set(LEAP_SDK_INCLUDE "${LEAP_SDK_PATH}/include")
set(LEAP_SDK_LIB "${LEAP_SDK_PATH}/lib/x64") # Assuming x64 architecture
set(LEAP_SDK_LIB_FILE "${LEAP_SDK_LIB}/LeapC.lib")

# Force all targets to use the dynamic runtime (/MD or /MDd)
foreach(flag_var
    CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "/MT")
    string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
  endif()
endforeach()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

# Force GTest and Catch2 to use dynamic CRT
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(CATCH_BUILD_STATIC_LIBRARY OFF CACHE BOOL "" FORCE)
set(CATCH_BUILD_SHARED_LIBRARY ON CACHE BOOL "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)

# GLM (for AspectMapper and math)
find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
      glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG 0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()
# Add GLM include directory after FetchContent
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/glm-src")
    message(STATUS "[GLOBAL] Adding GLM include directory: ${CMAKE_BINARY_DIR}/_deps/glm-src")
    include_directories(BEFORE SYSTEM "${CMAKE_BINARY_DIR}/_deps/glm-src")
else()
    message(WARNING "[GLOBAL] GLM include directory does not exist at: ${CMAKE_BINARY_DIR}/_deps/glm-src")
endif()

# Catch2 (for AspectMapper tests)
find_package(Catch2 2 QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
      catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v2.13.10
    )
    FetchContent_MakeAvailable(catch2)
endif()

# GTest (for DataProcessor and other tests)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()

# SDL2 (for UI tests)
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
      sdl2
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-2.26.5
    )
    FetchContent_MakeAvailable(sdl2)
endif()

# ImGui (for UI)
find_package(OpenGL REQUIRED)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

# Define OSCPack directory (Adjust if necessary)
set(OSCPACK_DIR "C:/Libraries/oscpack/oscpack-master" CACHE PATH "Path to oscpack library")

# --- Fetch Content Dependencies ---
# Make dependencies available
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(sdl2)
FetchContent_MakeAvailable(imgui)

# Set core ImGui source files
set(IMGUI_CORE_SRCS
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

# Add source and header directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/pipeline)
include_directories(${CMAKE_SOURCE_DIR}/src/ui)
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/src/transport)

# Gather all core, pipeline, ui, and transport sources
file(GLOB CORE_SRCS src/core/*.cpp)
file(GLOB PIPELINE_SRCS src/pipeline/*.cpp)
# Explicitly list UI sources to exclude _OLD files
set(UI_SRCS 
    src/ui/MainAppWindow.cpp
    src/ui/OpenGLRenderer.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
)
file(GLOB TRANSPORT_SRCS src/transport/osc/*.cpp)
# Ensure  is included explicitly
list(APPEND CORE_SRCS src/core/)

# Build a static library including core, pipeline, ui, and transport logic
add_library(leapmotion_core STATIC 
    ${CORE_SRCS} 
    ${PIPELINE_SRCS}
    ${UI_SRCS}          # Added UI sources
    ${TRANSPORT_SRCS}   # Added Transport OSC sources
    ${IMGUI_CORE_SRCS} # Add core ImGui sources
)

# Add include directories for leapmotion_core
target_include_directories(leapmotion_core PUBLIC
    "src"  # Changed from "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${LEAP_SDK_INCLUDE}
    ${GLM_INCLUDE_PATH}
    "${CMAKE_BINARY_DIR}/_deps/sdl2-src/include"
    "${OSCPACK_DIR}"
    "${OSCPACK_DIR}/ip"
    "${OSCPACK_DIR}/osc"
    "${imgui_SOURCE_DIR}" # Add fetched ImGui include path
    "${imgui_SOURCE_DIR}/backends" # Add fetched ImGui backends include path
    ${OPENGL_INCLUDE_DIR} # Add OpenGL include dir
)

target_link_directories(leapmotion_core PUBLIC ${LEAP_SDK_LIB})

get_target_property(_incdirs leapmotion_core INCLUDE_DIRECTORIES)
message(STATUS "leapmotion_core include dirs: ${_incdirs}")

# Link leapmotion_core against required libraries
target_link_libraries(leapmotion_core PRIVATE 
    oscpack_lib # Link against our new oscpack library
    glm 
    SDL2::SDL2-static # Link against fetched SDL2 static library
    ${LEAP_SDK_LIB_FILE} # Link against LeapC library
    OpenGL::GL # Link against OpenGL
)

# Add oscpack library target
add_library(oscpack_lib STATIC)

# Add oscpack source files to the library using the correct variable
target_sources(oscpack_lib PRIVATE
    "${OSCPACK_DIR}/ip/IpEndpointName.cpp"
    "${OSCPACK_DIR}/ip/win32/NetworkingUtils.cpp" # Assuming win32 subdir
    "${OSCPACK_DIR}/ip/win32/UdpSocket.cpp"       # Assuming win32 subdir
    "${OSCPACK_DIR}/osc/OscOutboundPacketStream.cpp"
    "${OSCPACK_DIR}/osc/OscPrintReceivedElements.cpp"
    "${OSCPACK_DIR}/osc/OscReceivedElements.cpp"
    "${OSCPACK_DIR}/osc/OscTypes.cpp"
)

# Set include directories for oscpack library itself
target_include_directories(oscpack_lib PUBLIC
    "${OSCPACK_DIR}"
    "${OSCPACK_DIR}/ip"
    "${OSCPACK_DIR}/osc"
)

# Main application executable
add_executable(leapMotionApp
    src/main.cpp
    src/ui/MainAppWindow.cpp
    src/ui/OpenGLRenderer.cpp
    src/ui/UIManager.cpp          # The new, correct one
    # src/ui/UIManager_OLD.cpp    # Explicitly NOT included
    # src/ui/imgui_impl_opengl3.cpp # Assuming these are needed for ImGui
    # src/ui/imgui_impl_sdl2.cpp    # Assuming these are needed for ImGui
    # Add any other necessary .cpp files from src/ or src/ui/ if missed
)
target_link_libraries(leapMotionApp PUBLIC leapmotion_core)
# Now that leapMotionApp exists, set its include directories
target_include_directories(leapMotionApp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"          # Added: Project source root
    "${CMAKE_BINARY_DIR}/_deps/sdl2-src/include"
    "${imgui_SOURCE_DIR}" # Add fetched ImGui include path
    "${imgui_SOURCE_DIR}/backends" # Add fetched ImGui backends include path
    "${OSCPACK_DIR}"                          # Added: oscpack base
    "${OSCPACK_DIR}/ip"                       # Added: oscpack ip
    "${OSCPACK_DIR}/osc"                      # Added: oscpack osc
)

# Add tests


add_executable(test_DataProcessor tests/test_DataProcessor.cpp)
target_include_directories(test_DataProcessor PRIVATE ${LEAP_SDK_INCLUDE})
# Try to link to both GTest::gtest and gtest, whichever exists
if(TARGET GTest::gtest AND TARGET GTest::gtest_main)
    message(STATUS "Linking test_DataProcessor to GTest::gtest and GTest::gtest_main")
    target_link_libraries(test_DataProcessor PRIVATE leapmotion_core GTest::gtest GTest::gtest_main glm ${LEAP_SDK_LIB_FILE})
elseif(TARGET gtest AND TARGET gtest_main)
    message(STATUS "Linking test_DataProcessor to gtest and gtest_main (non-namespaced)")
    target_link_libraries(test_DataProcessor PRIVATE leapmotion_core gtest gtest_main glm ${LEAP_SDK_LIB_FILE})
else()
    message(FATAL_ERROR "Neither GTest::gtest nor gtest targets are available!")
endif()
add_test(NAME test_DataProcessor COMMAND test_DataProcessor)

# Add other test executables as needed
file(GLOB TEST_SRCS tests/test_*.cpp)
foreach(test_src ${TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    if(NOT (test_name STREQUAL "test_AspectMapper" OR test_name STREQUAL "test_DataProcessor"))
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE ${LEAP_SDK_INCLUDE})
        target_link_libraries(${test_name} PRIVATE leapmotion_core GTest::gtest GTest::gtest_main glm ${LEAP_SDK_LIB_FILE})
        if(test_name STREQUAL "test_MainAppWindow")
            target_link_libraries(${test_name} PRIVATE SDL2::SDL2)
        endif()
    endif()
endforeach()

# Enable testing
include(CTest)
enable_testing()
