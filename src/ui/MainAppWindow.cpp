#include "MainAppWindow.h" // Must be first project header
#include "../core/Log.hpp" // Added include for Log
#include "../pipeline/03_DataProcessor.hpp"   // brings in DataProcessor + AspectPreset

// Standard Library Includes
#include <iostream>     // For std::cerr (may be removed if logger fully replaces)
#include <sstream>      // For std::ostringstream (if used)
#include <stdexcept>    // For std::exception (if used)
#include <vector>       // Used by status messages
#include <string>       // Used extensively
#include <chrono>       // Used in handleTrackingData
#include <utility>      // For std::move, std::forward_as_tuple
#include <tuple>        // For std::forward_as_tuple
#include <set>          // Used previously, check if still needed
#include <algorithm>    // Used previously, check if still needed
#include <mutex>        // Used for locking

// SDL Includes
#include <SDL.h>
#include <SDL_syswm.h> // Required for SDL_GetWindowWMInfo

// OpenGL Includes (using SDL's OpenGL header)
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h> // Include before SDL_opengl.h on Windows
#undef max
#undef min
#endif
#include <SDL_opengl.h>
#include <GL/gl.h> 

// ImGui Includes (via wrapper)
#include "ImGuiWrapper.h" // Includes imgui.h, imgui_impl_sdl2.h, imgui_impl_opengl3.h, ImGuiSafety.h"

// Project Internal Includes
#include "../core/ConfigManager.h" // Needed for OSC Settings panel
#include "transport/osc/OscController.h" // Needed for OSC Settings panel
#include "UIController.hpp" // Needed for interactions
#include "json.hpp" // Ensure json is included here
#include "../core/ConnectEvent.hpp"
#include "../core/DisconnectEvent.hpp"
#include "../core/DeviceConnectedEvent.hpp"
#include "../core/DeviceLostEvent.hpp"
#include "../core/DeviceHandAssignedEvent.hpp" // Already included

#ifdef _DEBUG
void MainAppWindow::checkGLError(const std::string& location, std::function<void(const std::string&)> logger) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::string errorStr;
        switch (err) {
            case GL_INVALID_ENUM:                  errorStr = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 errorStr = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             errorStr = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                errorStr = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               errorStr = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 errorStr = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               errorStr = "UNKNOWN_ERROR (" + std::to_string(err) + ")"; break;
        }
        if (logger) {
             logger("OpenGL Error at " + location + ": " + errorStr);
        } else {
            // Fallback if logger isn't available (shouldn't happen here)
            OutputDebugStringA(("OpenGL Error at " + location + ": " + errorStr + "\n").c_str());
        }
    }
}
#endif

MainAppWindow::MainAppWindow(
    std::function<void(const FrameData&)> onTrackingData,
    std::function<void(const ConnectEvent&)> onConnect,
    std::function<void(const DisconnectEvent&)> onDisconnect,
    std::function<void(const DeviceConnectedEvent&)> onDeviceConnected,
    std::function<void(const DeviceLostEvent&)> onDeviceLost,
    std::function<void(const DeviceHandAssignedEvent&)> onDeviceHandAssigned,
    std::function<void(const std::string&)> logger
) : onTrackingData(onTrackingData),
    onConnect(onConnect),
    onDisconnect(onDisconnect),
    onDeviceConnected(onDeviceConnected),
    onDeviceLost(onDeviceLost),
    onDeviceHandAssigned(onDeviceHandAssigned),
    logger_(std::move(logger))
{
    // Constructor body (if any)
    LOG("MainAppWindow constructed.");
}

MainAppWindow::~MainAppWindow() { 
    LOG("MainAppWindow shutting down...");
    shutdown(); 
}

SDL_Window* MainAppWindow::getWindow() {
    return window_.get();
}

void MainAppWindow::setControllers(ConfigManagerInterface* configMgr, OscController* oscCtrl) {
    configManager = configMgr;
    oscController = oscCtrl;
    if(configManager && oscController) {
        LOG("Successfully set ConfigManager and OscController in MainAppWindow.");
    } else {
        if (!configManager) {
            LOG_ERR("ConfigManager pointer is null in setControllers.");
        }
        if (!oscController) {
            LOG_ERR("OscController pointer is null in setControllers.");
        }
    }
    // TODO: Potentially validate pointers are not null
    // Log::getInstance()->info("Setting controllers/managers."); // Keep original for now if needed
    // DeviceManager removed: use LeapPoller or new pipeline for device enumeration
    
    // Load OSC filter settings from ConfigManager - REMOVED THIS BLOCK
    // if (configManager) {
    //     sendPalm = configManager->getBool("osc_filter_palm", true); 
    //     sendWrist = configManager->getBool("osc_filter_wrist", false);
    //     sendThumb = configManager->getBool("osc_filter_thumb", false);
    //     sendIndex = configManager->getBool("osc_filter_index", false);
    //     sendMiddle = configManager->getBool("osc_filter_middle", false);
    //     sendRing = configManager->getBool("osc_filter_ring", false);
    //     sendPinky = configManager->getBool("osc_filter_pinky", false);
    // }
    
    // Subscribe to events now that we have the controllers
    this->subscribeToEvents(); // Assuming this method exists and is correct
    LOG("Controllers set for MainAppWindow.");
}

bool MainAppWindow::init(const char* title, int width = 1280, int height = 720) { 
    LOG("Initializing MainAppWindow...");
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        logger_(std::string("Error initializing SDL: ") + SDL_GetError());
        return false;
    }

    // 2. Set OpenGL attributes (These must be set BEFORE creating the window)
    // Restoring attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // 3. Create SDL Window (with OpenGL flag)
    SDL_Window* rawWindow = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN
    );
    window_ = std::unique_ptr<SDL_Window, SdlWindowDeleter>(rawWindow); // Use custom deleter

    // 4. Check window creation failure (removing debug pause)
    if (!window_) {
        std::string errorMsg = std::string("SDL_CreateWindow Error: ") + SDL_GetError();
        logger_(errorMsg);
        // Removed debug output and pause
        SDL_Quit();
        return false;
    }
    // Initialize the OpenGL renderer
    if (!renderer.init(window_.get())) {
        logger_("Failed to initialize OpenGL renderer");
        // Need to decide if window/SDL should be cleaned up here if renderer fails
        window_.reset();
        SDL_Quit();
        return false;
    }

    // +++ Disable VSync +++
    if (SDL_GL_SetSwapInterval(0) < 0) {
        logger_(std::string("Warning: Unable to disable VSync! SDL Error: ") + SDL_GetError());
    } else {
        logger_("VSync disabled.");
    }
    // +++ End VSync Disable +++

    // Initialize ImGui
    if (!initImGui()) {
        LOG_ERR("Failed to initialize ImGui");
        return false;
    }

    // Initialize width and height member variables
    windowWidth = width;
    windowHeight = height;

    // --- Get and Store HWND --- 
#if defined(_WIN32)
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window_.get(), &wmInfo)) {
        if (wmInfo.subsystem == SDL_SYSWM_WINDOWS) { // Ensure it's the Windows subsystem
            nativeWindowHandle_ = wmInfo.info.win.window;
            if(nativeWindowHandle_){
                logger_("Successfully retrieved native HWND for the main window.");
            } else {
                logger_("Warning: SDL_GetWindowWMInfo subsystem WIN successful but HWND was NULL.");
            }
        } else {
             logger_("Warning: SDL_GetWindowWMInfo subsystem is not Windows.");
        }
    } else {
        logger_(std::string("Warning: Could not get window WM Info: ") + SDL_GetError());
    }
#else
    logger_("Info: Not on Windows, skipping native HWND retrieval.");
#endif
    // --- End HWND --- 

    LOG("MainAppWindow initialized successfully.");
    return true;
}

void MainAppWindow::shutdown() {
    LOG("MainAppWindow shutdown called.");
    // ... shutdown logic ...
    // Save OSC filter settings - REMOVED as part of config consolidation
    // if (configManager) { ... }
    
    // Shutdown ImGui
    shutdownImGui();
    // Shutdown OpenGL renderer
    renderer.shutdown();
    // Destroy window
    window_.reset();
    SDL_Quit();
    LOG("MainAppWindow shut down.");
}

// Event processing
void MainAppWindow::processEvent(SDL_Event* event) {
    // Let ImGui process the event first
    ImGui_ImplSDL2_ProcessEvent(event);
    
    // Handle window events (Example from previous UIManager code)
    if (event->type == SDL_WINDOWEVENT) {
        if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
            windowWidth = event->window.data1;
            windowHeight = event->window.data2;
            renderer.handleResize(windowWidth, windowHeight);
        }
    }
    // Add any other non-ImGui event processing here if needed
}

bool MainAppWindow::initImGui()
{
    if (imguiInitialized) {
        logger_("initImGui called but ImGui is already initialized! Skipping.");
        return false;
    }

    if (!window_) {
        logger_("ERROR: Window is null before ImGui init");
    }
    if (!renderer.getGLContext()) {
        logger_("ERROR: GL context is null before ImGui init");
    }
    logger_("Window pointer value: " + std::to_string(reinterpret_cast<uintptr_t>(window_.get())));
    logger_("GL Context pointer value: " + std::to_string(reinterpret_cast<uintptr_t>(renderer.getGLContext())));
    logger_("Calling ImGui_ImplSDL2_InitForOpenGL...");
    // Check ImGui version and Create context
    IMGUI_CHECKVERSION();
    if (!ImGui::CreateContext()) {
        LOG_ERR("Failed to create ImGui context");
        return false;
    }

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Minimal tweak pack: calm, rounded, muted palette
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding   = 6.0f;
    s.FrameRounding    = 4.0f;
    s.GrabRounding     = 4.0f;
    s.TabRounding      = 4.0f;
    s.WindowPadding    = ImVec2(14, 12);
    s.FramePadding     = ImVec2(10, 6);
    s.ItemSpacing      = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.Colors[ImGuiCol_WindowBg]        = ImColor(30, 32, 38, 230);
    s.Colors[ImGuiCol_FrameBg]         = ImColor(44, 48, 58, 255);
    s.Colors[ImGuiCol_FrameBgHovered]  = ImColor(66, 70, 80, 255);
    s.Colors[ImGuiCol_FrameBgActive]   = ImColor(80, 84, 94, 255);
    s.Colors[ImGuiCol_Button]          = ImColor(52, 56, 66, 255);
    s.Colors[ImGuiCol_ButtonHovered]   = ImColor(72, 76, 86, 255);
    s.Colors[ImGuiCol_ButtonActive]    = ImColor(92, 96, 106, 255);
    // Make all blue-tinted elements green
    s.Colors[ImGuiCol_Header]          = ImColor(60, 120, 60, 200);      // Green
    s.Colors[ImGuiCol_HeaderHovered]   = ImColor(80, 180, 80, 220);      // Lighter green
    s.Colors[ImGuiCol_HeaderActive]    = ImColor(100, 220, 100, 255);    // Bright green
    s.Colors[ImGuiCol_SeparatorHovered]= ImColor(80, 180, 80, 220);      // Green
    s.Colors[ImGuiCol_SeparatorActive] = ImColor(100, 220, 100, 255);    // Green
    s.Colors[ImGuiCol_SliderGrab]      = ImColor(80, 180, 80, 220);      // Green
    s.Colors[ImGuiCol_SliderGrabActive]= ImColor(100, 220, 100, 255);    // Green
    s.Colors[ImGuiCol_CheckMark]       = ImColor(80, 220, 80, 255);      // Green
    s.Colors[ImGuiCol_TextSelectedBg]  = ImColor(60, 120, 60, 120);      // Green highlight
    s.Colors[ImGuiCol_DragDropTarget]  = ImColor(80, 220, 80, 230);      // Green
    s.Colors[ImGuiCol_NavHighlight]    = ImColor(80, 220, 80, 255);      // Green
    s.Colors[ImGuiCol_NavWindowingHighlight] = ImColor(80, 220, 80, 180); // Green
    s.Colors[ImGuiCol_NavWindowingDimBg]    = ImColor(30, 60, 30, 120);   // Muted green


    // Setup Platform/Renderer backends
    if (!window_ || !renderer.getGLContext()) {
        LOG_ERR("Cannot initialize ImGui backends: Window or GL context is null.");
        ImGui::DestroyContext(); // Clean up context if backends fail
        return false;
    }

    // Initialize SDL2 backend
    if (!ImGui_ImplSDL2_InitForOpenGL(window_.get(), renderer.getGLContext())) {
        LOG_ERR("Failed to initialize ImGui SDL2 backend");
        ImGui::DestroyContext();
        return false;
    }

    // Initialize OpenGL3 backend
    const char* glsl_version = "#version 330 core";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        LOG_ERR("Failed to initialize ImGui OpenGL3 backend");
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    imguiInitialized = true;
    LOG("ImGui Initialized");
    return true;
}

void MainAppWindow::shutdownImGui() {
    if (imguiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
        LOG("ImGui backends and context shut down.");
    }
}

void MainAppWindow::render() { 
    if (ImGui::GetCurrentContext() == nullptr) {
        // ImGui context is null; logger_ must NOT log to an ImGui widget here!
        if (logger_) {
            // If logger_ is a file/stdout logger, this is safe.
            logger_("Error: ImGui context is null in render(). Cannot render.");
        } else {
            std::cerr << "Error: ImGui context is null in render(). Cannot render." << std::endl;
        }
        // Maybe add a simple background clear here as fallback?
        // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT);
        // SDL_GL_SwapWindow(window);
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(); // Pass SDL window implicitly
    ImGui::NewFrame();

    // Render the main UI elements
    renderMainUI(); // Call member function directly

    // Finalize ImGui rendering for the frame
    ImGui::Render();

    // Get OpenGL Viewport and Clear
    // Note: Assuming renderer manages viewport settings
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f); // Example clear color
    glClear(GL_COLOR_BUFFER_BIT);
    #ifdef _DEBUG
    checkGLError("after glClear", logger_);
    #endif

    // Render ImGui draw data using OpenGL3 backend
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data != nullptr && draw_data->CmdListsCount > 0) {
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        #ifdef _DEBUG
        checkGLError("after ImGui_ImplOpenGL3_RenderDrawData", logger_);
        #endif
    } else {
         // Only log if context exists but draw data is still null/empty - might indicate no UI was drawn
         if(logger_ && ImGui::GetCurrentContext()) {
              logger_("Warning: ImGui draw data is null or empty after ImGui::Render(). No UI drawn?");
         }
    }

    // Swap the window buffer
    if (window_) {
        SDL_GL_SwapWindow(window_.get());
        #ifdef _DEBUG
        checkGLError("after SDL_GL_SwapWindow", logger_);
        #endif
    } else {
        logger_("Error: SDL_Window is null in render(). Cannot swap buffers.");
    }
}

void MainAppWindow::renderMainUI()
{
    auto* concreteConfig = dynamic_cast<ConfigManager*>(configManager);
    bool configAvailable = (concreteConfig != nullptr);
    bool processorAvailable = (dataProcessor != nullptr);

    // Create a main window that fills the entire app window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight)));
    ImGui::Begin("LeapApp Control Panel", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

    // Menu bar
    renderMenuBar();

    renderDevicePanel();
    renderOscSettingsPanel();
    renderStatusMessagesPanel();
    renderAboutPanel();

    ImGui::End();
}

/**
 * Must be called before first render() to ensure dataProcessor is valid.
 */
void MainAppWindow::setDataProcessor(DataProcessor* processor) {
    dataProcessor = processor;
}

void MainAppWindow::renderMenuBar() {
    // Check if the menu bar should be drawn
    if (ImGui::BeginMenuBar()) { 
        // File Menu
        if (ImGui::BeginMenu("File")) {
            // Removed commented out Save/Load settings
            if (ImGui::MenuItem("Exit")) {
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    } // End of BeginMenuBar check
}

// ... rest of the helper methods ... 

// --- Private Method Implementations ---

// Placeholder implementation - Add actual event subscription logic if needed
void MainAppWindow::subscribeToEvents() {
    logger_("MainAppWindow::subscribeToEvents() called - TODO: Implement logic.");
    // Example: register event handlers with an event manager if you have one
}

void MainAppWindow::renderDevicePanel()
{
    if (!ImGui::CollapsingHeader("Devices", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // --- Compute table height for content-fit panel ---
    const int rowCount   = static_cast<int>(deviceTrackingDataMap.size());
    const float rowH     = ImGui::GetFrameHeightWithSpacing();
    const float hdrH     = rowH; // table header ≈ one row
    const float padding  = ImGui::GetStyle().WindowPadding.y * 2.0f;
    const float tableH   = hdrH + rowCount * rowH + padding;
    ImGui::BeginChild("##DevicePanel", ImVec2(-FLT_MIN, tableH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    //---------------------------------------------------------------
    // 1. Take a snapshot of the device data, *then* release mutex
    //---------------------------------------------------------------
    std::vector<DeviceRowDisplayData> displayList; // Using the existing struct
    {
        std::lock_guard<std::mutex> lock(trackingDataMutex);
        displayList.reserve(deviceTrackingDataMap.size());

        for (const auto& [serial, data] : deviceTrackingDataMap)
        { // Use const auto& since we only need to read
            std::string alias = aliasLookupFunc_ ? aliasLookupFunc_(serial) : "N/A";
            if (alias.empty()) alias = "-";
            displayList.push_back({serial, alias, &data}); // Copy data needed for display
        }
    } // Mutex is released here

    // Sort the local copy
    std::sort(displayList.begin(), displayList.end(), [](const DeviceRowDisplayData& a, const DeviceRowDisplayData& b) {
        bool aIsDev = a.alias.rfind("dev", 0) == 0;
        bool bIsDev = b.alias.rfind("dev", 0) == 0;
        if (aIsDev && bIsDev) {
            try {
                int aNum = std::stoi(a.alias.substr(3));
                int bNum = std::stoi(b.alias.substr(3));
                return aNum < bNum;
            } catch (...) {
                return a.alias < b.alias;
            }
        } else if (aIsDev) {
            return true;
        } else if (bIsDev) {
            return false;
        } else {
            return a.alias < b.alias;
        }
    });

    //---------------------------------------------------------------
    // 2. Render table from the snapshot (no lock held)
    //---------------------------------------------------------------
    if (!ImGui::BeginTable("DevicesTable", 8, // Adjusted column count
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        return; // Exit if table creation fails
    }

    // Setup columns (as before)
    ImGui::TableSetupColumn("Serial", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Alias", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Assigned Hand", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Hand Count", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Frame Rate", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Strength", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Assign Hand", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    // Local queue for pending assignments during this render pass
    std::vector<std::pair<std::string, std::string>> pendingAssignments;

    // Render rows from the snapshot
    for (const auto& row : displayList)
    {
        ImGui::TableNextRow();

        // Render columns 0-6 (as before, using row.serial, row.alias, row.deviceData)
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(row.serial.c_str());
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", row.alias.c_str());
        ImGui::TableSetColumnIndex(2); 
        if (row.deviceData->isConnected) {
             ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); ImGui::Text("Connected"); ImGui::PopStyleColor();
        } else {
             ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); ImGui::Text("Disconnected"); ImGui::PopStyleColor();
        }
        ImGui::TableSetColumnIndex(3); 
        std::string handStr = DeviceHandAssignedEvent::handTypeToString(row.deviceData->assignedHand);
        ImGui::TextUnformatted(handStr.empty() ? "None" : handStr.c_str());
        ImGui::TableSetColumnIndex(4); ImGui::Text("%d", row.deviceData->handCount.load());
        ImGui::TableSetColumnIndex(5); ImGui::Text("%.1f Hz", row.deviceData->frameRate.load());
        ImGui::TableSetColumnIndex(6);
        ImGui::Text("L: P:%.2f G:%.2f", row.deviceData->leftPinchStrength.load(), row.deviceData->leftGrabStrength.load());
        ImGui::SameLine(); 
        ImGui::Text("R: P:%.2f G:%.2f", row.deviceData->rightPinchStrength.load(), row.deviceData->rightGrabStrength.load());

        // --- Hand Assignment Dropdown (Column 7) --- 
        ImGui::TableSetColumnIndex(7);
        ImGui::PushID(row.serial.c_str()); // Unique ID per row
        static const char* handOptions[] = { "None", "Left", "Right" };
        int currentHand = 0;
        switch (row.deviceData->assignedHand) {
            case DeviceHandAssignedEvent::HandType::HAND_LEFT: currentHand = 1; break;
            case DeviceHandAssignedEvent::HandType::HAND_RIGHT: currentHand = 2; break;
            case DeviceHandAssignedEvent::HandType::HAND_NONE:
            default: currentHand = 0; break;
        }
        int prevHand = currentHand;
        std::string comboId = std::string("##assignHand_") + row.serial;
        if (ImGui::Combo(comboId.c_str(), &currentHand, handOptions, IM_ARRAYSIZE(handOptions))) {
            // Only queue if changed
            if (currentHand != prevHand) {
                std::string handStr = (currentHand == 1) ? "LEFT" : (currentHand == 2) ? "RIGHT" : "NONE";
                logger_("Queueing " + handStr + " assign for: " + row.serial);
                pendingAssignments.emplace_back(row.serial, handStr);
            }
        }
        ImGui::PopID(); // Pop ID for the row

    }

    ImGui::EndTable();
    ImGui::EndChild();

    //---------------------------------------------------------------
    // 3. Apply the requested assignments *after* the table is closed
    //---------------------------------------------------------------
    if (uiController_) {
        for (const auto& [serial, hand] : pendingAssignments) {
            logger_("Processing assignment: " + serial + " -> " + hand);
            uiController_->setDeviceHandAssignment(serial, hand);
        }
    } else if (!pendingAssignments.empty()) {
         logger_("Warning: UIController missing, cannot process pending assignments.");
    }
}

void MainAppWindow::renderOscSettingsPanel() {
    if (!uiController_) {
        ImGui::Text("OSC Settings unavailable (UI Controller missing).");
        return;
    }

    ImGui::BeginChild("OSCSettings", ImVec2(0, 0), true);
    ImGui::Text("OSC Target");
    ImGui::PushItemWidth(150);
    
    char* oscIpBuffer = uiController_->getOscIpBuffer();
    if (ImGui::InputText("IP Address", oscIpBuffer, UIController::OSC_IP_BUFFER_SIZE)) {
        // Assuming changes are applied via applyOscSettings button
        // uiController_->handleOscIpChanged(); 
    }
    ImGui::SameLine();
    int* oscPortPtr = uiController_->getOscPortPtr();
    if (ImGui::InputInt("Port", oscPortPtr)) {
        if (*oscPortPtr < 0) *oscPortPtr = 0;
        if (*oscPortPtr > 65535) *oscPortPtr = 65535;
        // Assuming changes are applied via applyOscSettings button
        // uiController_->handleOscPortChanged(); 
    }
    ImGui::PopItemWidth();
    // Add Apply button if needed
    if (ImGui::Button("Apply OSC Settings")) {
        uiController_->applyOscSettings(); 
    }

    ImGui::Separator();
    ImGui::Text("OSC Data Filters");

    // Fetch current states using original getter names
    bool sendPalm = uiController_->isPalmFilterEnabled();
    bool sendWrist = uiController_->isWristFilterEnabled();
    bool sendThumb = uiController_->isThumbFilterEnabled();
    bool sendIndex = uiController_->isIndexFilterEnabled();
    bool sendMiddle = uiController_->isMiddleFilterEnabled();
    bool sendRing = uiController_->isRingFilterEnabled();
    bool sendPinky = uiController_->isPinkyFilterEnabled();
    // Get states for other original filters if they exist
    bool sendPalmOrientation = uiController_->isPalmOrientationFilterEnabled();
    bool sendPalmVelocity = uiController_->isPalmVelocityFilterEnabled();
    bool sendPalmNormal = uiController_->isPalmNormalFilterEnabled();
    bool sendVisibleTime = uiController_->isVisibleTimeFilterEnabled();
    bool sendFingerIsExtended = uiController_->isFingerIsExtendedFilterEnabled();
    // Get states for pinch/grab filters
    bool sendPinchStrength = uiController_->isPinchStrengthFilterEnabled();
    bool sendGrabStrength = uiController_->isGrabStrengthFilterEnabled();

    // Render checkboxes and update UIController on change using setFilterState
    if (ImGui::Checkbox("Send Palm", &sendPalm)) {
        uiController_->setFilterState("sendPalm", sendPalm);
    }
    if (ImGui::Checkbox("Send Wrist", &sendWrist)) {
        uiController_->setFilterState("sendWrist", sendWrist);
    }
    if (ImGui::Checkbox("Send Thumb", &sendThumb)) {
        uiController_->setFilterState("sendThumb", sendThumb);
    }
    if (ImGui::Checkbox("Send Index Finger", &sendIndex)) {
        uiController_->setFilterState("sendIndex", sendIndex);
    }
    if (ImGui::Checkbox("Send Middle Finger", &sendMiddle)) {
        uiController_->setFilterState("sendMiddle", sendMiddle);
    }
    if (ImGui::Checkbox("Send Ring Finger", &sendRing)) {
        uiController_->setFilterState("sendRing", sendRing);
    }
    if (ImGui::Checkbox("Send Pinky Position", &sendPinky)) {
        uiController_->setFilterState("sendPinky", sendPinky);
    }
    if (ImGui::Checkbox("Send Finger Is Extended", &sendFingerIsExtended)) {
        uiController_->setFilterState("sendFingerIsExtended", sendFingerIsExtended);
    }
    // Add checkboxes for other original filters
    if (ImGui::Checkbox("Send Palm Orientation", &sendPalmOrientation)) {
        uiController_->setFilterState("sendPalmOrientation", sendPalmOrientation);
    }
    if (ImGui::Checkbox("Send Palm Velocity", &sendPalmVelocity)) {
        uiController_->setFilterState("sendPalmVelocity", sendPalmVelocity);
    }
    if (ImGui::Checkbox("Send Palm Normal", &sendPalmNormal)) {
        uiController_->setFilterState("sendPalmNormal", sendPalmNormal);
    }
    if (ImGui::Checkbox("Send Visible Time", &sendVisibleTime)) {
        uiController_->setFilterState("sendVisibleTime", sendVisibleTime);
    }
    
    // Add checkboxes for pinch/grab filters
    if (ImGui::Checkbox("Send Pinch Strength", &sendPinchStrength)) {
        uiController_->setFilterState("sendPinchStrength", sendPinchStrength);
    }
    if (ImGui::Checkbox("Send Grab Strength", &sendGrabStrength)) {
        uiController_->setFilterState("sendGrabStrength", sendGrabStrength);
    }

    // --- REMOVED Pinch/Grab Checkboxes --- -> Re-added above
    // --- REMOVED Session Duration Display and Reset Button --- 

    ImGui::EndChild();
}

// Implementation was previously added by user edit
void MainAppWindow::renderStatusMessagesPanel() {
    if (ImGui::CollapsingHeader("Status Messages")) { // Use CollapsingHeader
        ImGui::Text("Status Messages Go Here...");
        ImGui::Spacing();
        // TODO: Implement display of status messages from statusMessages vector
    } // End of CollapsingHeader block
}

// Implementation was previously added by user edit
void MainAppWindow::renderAboutPanel() {
    if (ImGui::CollapsingHeader("About")) { // Use CollapsingHeader
        ImGui::Text("LeapApp vX.Y - About Info Goes Here...");
        ImGui::Spacing();
        // TODO: Add version info, credits, links etc.
    } // End of CollapsingHeader block
}

// Placeholder or existing implementation
bool MainAppWindow::recreateRenderer() {
    // ... implementation ...
    return true; // Assuming it returns bool
}

// Implementation was previously added
void MainAppWindow::addStatusMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(statusMutex);
    statusMessages.push_back(message);
    const size_t MAX_STATUS_MESSAGES = 100;
    if (statusMessages.size() > MAX_STATUS_MESSAGES) {
        statusMessages.erase(statusMessages.begin());
    }
}

// Implementation was previously added
std::vector<std::string> MainAppWindow::getStatusMessages() {
    std::lock_guard<std::mutex> lock(statusMutex);
    return statusMessages; 
}

// Implementation was previously added
MainAppWindow::PerDeviceTrackingData& MainAppWindow::getDeviceData(const std::string& serialNumber) {
    // Lock should be acquired by the CALLER before calling this function
    auto [it, inserted] = deviceTrackingDataMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(serialNumber),
        std::forward_as_tuple()
    );
    if (inserted) {
        it->second.serialNumber = serialNumber;
    }
    return it->second;
}

const MainAppWindow::PerDeviceTrackingData& MainAppWindow::getDeviceData(const std::string& serialNumber) const {
    auto it = deviceTrackingDataMap.find(serialNumber);
    if (it == deviceTrackingDataMap.end()) {
        throw std::out_of_range("Device not found: " + serialNumber);
    }
    return it->second;
}

// Event handlers - UPDATED implementation signature and logic
void MainAppWindow::handleTrackingData(const FrameData& frame) {
    if (logger_) logger_("MainAppWindow::handleTrackingData received frame");

    std::lock_guard<std::mutex> lock(trackingDataMutex); 
    MainAppWindow::PerDeviceTrackingData& data = getDeviceData(frame.deviceId); 

    auto now = std::chrono::high_resolution_clock::now();
    if (data.frameCount > 0) { 
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - data.lastFrameTime).count();
        const long long MIN_DURATION_US = 1000; // Corresponds to 1000 FPS max instantaneous
        if (duration >= MIN_DURATION_US) { // Check if duration is reasonable
            double currentFps = 1000000.0 / duration;
            double alpha = 0.1; 
            data.frameRate.store(alpha * currentFps + (1.0 - alpha) * data.frameRate.load()); 
        } // else: duration is too small, don't update FPS this cycle
    }
    data.lastFrameTime = now;
    data.frameCount++;

    data.handCount.store(static_cast<int>(frame.hands.size()));

    // Reset pinch/grab before updating
    float lPinch = 0.0f, lGrab = 0.0f, rPinch = 0.0f, rGrab = 0.0f;
    for (const auto& hand : frame.hands) {
        if (hand.handType == "left") {
            lPinch = hand.pinchStrength;
            lGrab = hand.grabStrength;
        } else if (hand.handType == "right") {
            rPinch = hand.pinchStrength;
            rGrab = hand.grabStrength;
        }
    }
    data.leftPinchStrength.store(lPinch);
    data.leftGrabStrength.store(lGrab);
    data.rightPinchStrength.store(rPinch);
    data.rightGrabStrength.store(rGrab);
}

void MainAppWindow::handleConnect(const ConnectEvent& event) {
    isLeapConnected.store(true);
    addStatusMessage("Connected to Leap Motion service");
}

void MainAppWindow::handleDisconnect(const DisconnectEvent& event) {
    isLeapConnected.store(false);
    addStatusMessage("Disconnected from Leap Motion service");
}

void MainAppWindow::handleDeviceConnected(const DeviceConnectedEvent& event) {
    std::string message = "Device connected: SN " + event.serialNumber;
    addStatusMessage(message);

    std::string postLockMessage;
    {
        std::lock_guard<std::mutex> lock(trackingDataMutex);
        MainAppWindow::PerDeviceTrackingData& data = getDeviceData(event.serialNumber);
        data.isConnected = true;
        postLockMessage = "Updated tracking data for connected device SN: " + event.serialNumber;
    }
    addStatusMessage(postLockMessage);
}

void MainAppWindow::handleDeviceLost(const DeviceLostEvent& event) {
    std::string message = "Device lost: SN " + event.serialNumber;
    addStatusMessage(message);

    std::vector<std::string> postLockMessages;
    {
        std::lock_guard<std::mutex> lock(trackingDataMutex);
        auto it = deviceTrackingDataMap.find(event.serialNumber);
        if (it != deviceTrackingDataMap.end()) {
            it->second.isConnected = false;
            postLockMessages.push_back("Marked device SN " + event.serialNumber + " as disconnected.");
        } else {
            if(logger_) logger_("WARN: Received disconnect for unknown device SN: " + event.serialNumber);
            postLockMessages.push_back("Received disconnect for unknown device SN: " + event.serialNumber);
        }
    }
    for (const auto& msg : postLockMessages) {
        addStatusMessage(msg);
    }
}

void MainAppWindow::handleDeviceHandAssigned(const DeviceHandAssignedEvent& event) {
    std::string handTypeStr = DeviceHandAssignedEvent::handTypeToString(event.handType);
    std::string message = "Device SN " + event.serialNumber + " assigned to " + handTypeStr;
    addStatusMessage(message);

    std::string postLockMessage;
    {
        std::lock_guard<std::mutex> lock(trackingDataMutex);
        MainAppWindow::PerDeviceTrackingData& data = getDeviceData(event.serialNumber);
        data.assignedHand = event.handType;
        postLockMessage = "Updated hand assignment for device SN: " + event.serialNumber;
    }
    addStatusMessage(postLockMessage);
}

// Method to set the UIController instance
void MainAppWindow::setUIController(UIController* controller) {
    uiController_ = controller;
    if (uiController_) {
        logger_("MainAppWindow: UIController instance set.");
    } else {
        logger_("WARN: MainAppWindow: UIController instance set to null.");
    }
}

// Add implementation for setting the alias lookup function
void MainAppWindow::setAliasLookupFunction(std::function<std::string(const std::string&)> func) {
    aliasLookupFunc_ = std::move(func);
    if (aliasLookupFunc_) {
        logger_("MainAppWindow: Alias lookup function set.");
    } else {
        logger_("WARN: MainAppWindow: Alias lookup function set to null.");
    }
}

// NEW: getHWND() implementation
#if defined(_WIN32)
HWND MainAppWindow::getHWND() {
    return nativeWindowHandle_;
}
#else
void* MainAppWindow::getHWND() {
    // Return a dummy value or nullptr for non-Windows
    return nullptr;
}
#endif 