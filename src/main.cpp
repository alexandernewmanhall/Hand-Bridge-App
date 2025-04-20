#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib") // Needed for Shell_NotifyIcon
#pragma comment(lib, "Dbghelp.lib") // Needed for crash dump generation

// Crash dump handler
#include "CrashDumpHandler.h"

// Include our warning suppression header first
#include "SuppressWarnings.h"

// Define minimum Windows version (e.g., Windows 10) BEFORE including windows.h
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif

// Windows API Headers
#include <windows.h> // Include the main header first
#include <winuser.h> // Explicitly include winuser.h for WaitMessageTimeout
#include <shellapi.h> // Required for Shell_NotifyIcon
// #include <windowsx.h> // Removed - Should be included by windows.h if needed

// Include resource definitions AFTER windows.h
#include "resource.h"

// Define custom message HERE instead of resource.h
#define WM_APP_TRAYMSG (WM_APP + 1)

#include <SDL.h>
#include <iostream>
#include <conio.h>        // For _kbhit and _getch functions (Consider removing if not needed)
#include <LeapC.h>        // The Ultraleap Tracking SDK C API
#include <string>         // For string operations
#include <atomic>         // For std::atomic
#include <SDL.h>     // SDL for window management
#include <vector>
#include <thread> // Include for std::this_thread
#include <chrono> // Include for std::chrono

// NEW: Include Service Locator and Logger Wrapper
#include "di/ServiceLocator.hpp"
#include "core/AppLogger.hpp"
// END NEW

// OSC Includes
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "pipeline/04_OscSender.hpp"    // Correct path for CURRENT OscSender
#include "transport/osc/OscController.h" // Keep if used DIRECTLY (check usage?)
#include "pipeline/01_LeapPoller.hpp"   // Use LeapPoller instead of LeapPollerStage
#include "pipeline/03_DataProcessor.hpp" // Use DataProcessor instead of DataProcessorStage

// Core App Includes
#include "core/ConfigManager.h"
#include "core/FrameData.hpp"       // Include FrameData for queue
#include "utils/SpscQueue.hpp"    // Include SpscQueue for queue
#include "app/AppCore.hpp"   // Include AppCore for pipeline management
#include "ui/MainAppWindow.h"     // Ensure MainAppWindow is defined

#include "core/DeviceAliasManager.hpp"   // Keep

// Include the RAII wrapper
#include "utils/HandleWrapper.hpp"

// Define specific handle types using the wrapper
inline BOOL DestroyIconWrapper(HICON hIcon) { return DestroyIcon(hIcon); } // Need wrapper for function pointer
inline BOOL DestroyMenuWrapper(HMENU hMenu) { return DestroyMenu(hMenu); } // Need wrapper for function pointer
using IconHandle = Handle<HICON, DestroyIconWrapper>;
using MenuHandle = Handle<HMENU, DestroyMenuWrapper>;

// --- Constants for Tray Icon --- defined in resource.h now ---
// #define IDI_APPICON 101
// #define IDM_OPEN 1001
// #define IDM_EXIT 1002

// --- Global Variables ---
HINSTANCE g_hInstance = NULL;
HWND g_hWndHidden = NULL; // Handle for the hidden message window
HWND g_hWndMain = NULL;   // Handle for the main SDL/ImGui window
bool g_isMainWindowVisible = false; // Track main window visibility
NOTIFYICONDATA g_notifyIconData; // Tray icon data (declare globally)

// --- Forward Declarations of Global Functions ---
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void ShowContextMenu(HWND hWnd);
// Modified ToggleMainWindowVisibility to accept ServiceLocator
void ToggleMainWindowVisibility(HWND hWndHidden, ServiceLocator& locator);
// REMOVED GetLeapRSString - seems unused here, can be moved if needed elsewhere
// static const char* GetLeapRSString(eLeapRS code);

// === Application Entry Point ===
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    EnableCrashDumps(); // Register crash dump handler immediately
    g_hInstance = hInstance;

    // +++ Initialize Service Locator +++
    ServiceLocator locator;
    std::shared_ptr<AppLogger> logger; // Declare logger pointer early
    std::shared_ptr<IConfigStore> configManager; // Use for AppCore compatibility
    std::shared_ptr<MainAppWindow> uiManager; // Declare early
    std::unique_ptr<AppCore> appCorePtr; // Use unique_ptr for RAII if AppCore manages resources

    // +++ Register Core Services and Initialize AppCore +++
    try {
        // Logger
        logger = std::make_shared<AppLogger>();
        locator.add(logger);
        logger->log("Logger service registered.");

        // ConfigManager
        auto concreteConfigManager = std::make_shared<ConfigManager>(); // Create concrete type
        configManager = concreteConfigManager; // Assign to IConfigStore pointer
        locator.add(configManager);
        // Need to register the concrete type too if other services need it specifically?
        // locator.add(concreteConfigManager); // Only if needed
        logger->log("ConfigManager service registered.");

        // MainAppWindow (UI Manager)
        uiManager = std::make_shared<MainAppWindow>(
            // Placeholder lambdas - These might need access to other services via the locator
            // or be refactored into AppCore/other classes later.
            [](const FrameData& frame){ /* Placeholder */ },
            [](const ConnectEvent&){ /* Placeholder */ },
            [](const DisconnectEvent&){ /* Placeholder */ },
            [](const DeviceConnectedEvent&){ /* Placeholder */ },
            [](const DeviceLostEvent&){ /* Placeholder */ },
            [](const DeviceHandAssignedEvent&){ /* Placeholder */ },
            // Pass the logger instance directly if needed during construction
            [logger](const std::string& msg) { logger->log(msg); }
            // Alternatively, modify MainAppWindow to get the logger from the locator if needed post-construction
        );
        locator.add(uiManager);
        logger->log("MainAppWindow service registered.");

        // Add pre-check for configManager before calling AppCore constructor
        if (!configManager) {
             logger->log("CRITICAL ERROR: configManager shared_ptr is NULL before AppCore construction!");
             throw std::runtime_error("configManager became null unexpectedly");
        }
        if (!logger) {
             // This shouldn't happen based on earlier checks, but good practice
             OutputDebugStringA("CRITICAL ERROR: logger shared_ptr is NULL before AppCore construction!\n");
             throw std::runtime_error("logger became null unexpectedly");
        }
        if (!uiManager) {
             logger->log("CRITICAL ERROR: uiManager shared_ptr is NULL before AppCore construction!");
             throw std::runtime_error("uiManager became null unexpectedly");
        }

        // AppCore Initialization (NOW in correct scope)
        logger->log("Initializing AppCore (with injected dependencies)...");

        // AppCore constructor no longer takes queue
        appCorePtr = std::make_unique<AppCore>(
            configManager,                 // Pass ConfigManager Interface
            *uiManager,                    // Pass MainAppWindow ref (check uiManager validity above)
            logger                         // Pass Logger
        );
        logger->log("AppCore initialized.");

        // --- Initialize mapping/gain/edge params from config ---
        // If AppCore exposes DataProcessor, set these values here. Otherwise, do it wherever DataProcessor is constructed.
        if (!appCorePtr) {
            logger->log("FATAL: AppCore pointer is null after initialization!");
            MessageBoxW(NULL, L"AppCore pointer is null after initialization!", L"Startup Error", MB_ICONSTOP | MB_OK);
            return 1;
        }
        auto* dataProcessor = appCorePtr->getDataProcessor();
        if (!dataProcessor) {
            logger->log("FATAL: DataProcessor pointer is null after AppCore construction!");
            MessageBoxW(NULL, L"DataProcessor pointer is null after AppCore construction!", L"Startup Error", MB_ICONSTOP | MB_OK);
            return 1;
        }
        // Set DataProcessor on MainAppWindow
        uiManager->setDataProcessor(dataProcessor);
        // Use dynamic_cast to get ConfigManager* for config-specific methods
        auto* concreteConfig = dynamic_cast<ConfigManager*>(configManager.get());
        if (!concreteConfig) {
            logger->log("FATAL: configManager is not a ConfigManager. Cannot initialize DataProcessor params.");
            MessageBoxW(NULL, L"ConfigManager is not a ConfigManager. Cannot initialize DataProcessor params.", L"Startup Error", MB_ICONSTOP | MB_OK);
            return 1;
        }
        // KEEP validation for gain/softzone
        float baseGain = concreteConfig->getBaseGain(), midGain = concreteConfig->getMidGain(), maxGain = concreteConfig->getMaxGain();
        float lowSpeedThresh = concreteConfig->getLowSpeedThreshold(), midSpeedThresh = concreteConfig->getMidSpeedThreshold();
        if (!(baseGain > 0 && midGain > 0 && maxGain > 0 && lowSpeedThresh > 0 && midSpeedThresh > 0)) {
            logger->log("WARNING: Invalid gain parameters in config. Using defaults.");
            baseGain = 1.0f; midGain = 3.0f; maxGain = 6.0f; lowSpeedThresh = 80.0f; midSpeedThresh = 240.0f;
            concreteConfig->setGainParams(baseGain, midGain, maxGain, lowSpeedThresh, midSpeedThresh);
        }
        // Removed soft zone width retrieval and setting
        // float softZone = concreteConfig->getSoftZoneWidth();
        // if (softZone < 0.01f || softZone > 0.25f) {
        //     logger->log("WARNING: Invalid soft zone width in config. Using default 0.10.");
        //     softZone = 0.10f;
        //     concreteConfig->setSoftZoneWidth(softZone);
        // }
        // Removed gain and soft zone settings
        // dataProcessor->setGainParams(baseGain, midGain, maxGain, lowSpeedThresh, midSpeedThresh);
        // dataProcessor->setSoftZone(softZone);
        logger->log("DataProcessor initialized from config."); // Updated log message

    } catch (const std::exception& e) {
        std::string errorMsg = "Core Initialization failed: ";
        errorMsg += e.what();
        // Try to use logger if available, otherwise direct message box
        if (logger) { logger->log("FATAL ERROR: " + errorMsg); }
        MessageBoxW(NULL, std::wstring(errorMsg.begin(), errorMsg.end()).c_str(), L"Initialization Error", MB_ICONEXCLAMATION | MB_OK);
        return 1; // Indicate failure
    }
    // +++ End Service Initialization +++

    // We should have a valid logger if we reached here
    if (!logger) { MessageBoxW(NULL, L"Logger unavailable after initialization!", L"Critical Error", MB_ICONSTOP | MB_OK); return 1; }
    // We must have a valid AppCore instance if initialization succeeded
    if (!appCorePtr) { logger->log("AppCore pointer is null after initialization block!"); MessageBoxW(NULL, L"AppCore failed to initialize!", L"Critical Error", MB_ICONSTOP | MB_OK); return 1; }

    // +++ START: Add Hidden Window and Tray Icon Creation +++
    logger->log("Creating hidden window and system tray icon...");

    // 1. Register Hidden Window Class
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandleW(NULL), NULL, NULL, NULL, NULL, L"LeapAppHiddenWindowClass", NULL };
    if (!RegisterClassExW(&wc)) {
        logger->log("ERROR: Failed to register hidden window class!");
        MessageBoxW(NULL, L"Failed to register hidden window class!", L"Startup Error", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    // 2. Create Hidden Message-Only Window (pass locator)
    g_hWndHidden = CreateWindowExW(
        0, L"LeapAppHiddenWindowClass", L"LeapApp Hidden", 0, 0, 0, 0, 0,
        HWND_MESSAGE, // Important: Message-only window
        NULL, g_hInstance,
        &locator // Pass locator pointer as creation parameter
        );

    if (!g_hWndHidden) {
        logger->log("ERROR: Failed to create hidden message window!");
        MessageBoxW(NULL, L"Failed to create hidden window!", L"Startup Error", MB_ICONEXCLAMATION | MB_OK);
        UnregisterClassW(wc.lpszClassName, wc.hInstance); // Clean up class registration
        return 1;
    }
    logger->log("Hidden message window created successfully.");

    // 3. Setup Tray Icon
    NOTIFYICONDATAW g_notifyIconDataW;
    ZeroMemory(&g_notifyIconDataW, sizeof(g_notifyIconDataW));
    g_notifyIconDataW.cbSize = sizeof(NOTIFYICONDATAW);
    g_notifyIconDataW.hWnd = g_hWndHidden;
    g_notifyIconDataW.uID = IDI_APPICON; // Use icon ID from resource.h
    g_notifyIconDataW.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notifyIconDataW.uCallbackMessage = WM_APP_TRAYMSG; // Custom message ID from resource.h

    // Load the icon (ensure IDI_APPICON is defined in resource.h and linked)
    g_notifyIconDataW.hIcon = (HICON)LoadImageW(g_hInstance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!g_notifyIconDataW.hIcon) {
        logger->log("ERROR: Failed to load application icon! Check resource.h and .rc file.");
        // Optionally provide a default icon or show an error
    } else {
         logger->log("Application icon loaded successfully.");
    }

    // Tooltip text
    lstrcpynW(g_notifyIconDataW.szTip, L"Leap Motion App", sizeof(g_notifyIconDataW.szTip)/sizeof(WCHAR));

    // 4. Add Icon to Tray
    if (!Shell_NotifyIconW(NIM_ADD, &g_notifyIconDataW)) {
         logger->log("ERROR: Failed to add icon to system tray!");
         // Clean up loaded icon if necessary
         if (g_notifyIconDataW.hIcon) DestroyIcon(g_notifyIconDataW.hIcon);
         DestroyWindow(g_hWndHidden); // Destroy the hidden window
         UnregisterClassW(wc.lpszClassName, wc.hInstance); // Clean up class registration
         MessageBoxW(NULL, L"Failed to add system tray icon!", L"Startup Error", MB_ICONEXCLAMATION | MB_OK);
         return 1;
     }
     logger->log("System tray icon added successfully.");
    // +++ END: Add Hidden Window and Tray Icon Creation +++

    // --- Initialize MainAppWindow (should be created hidden now) ---
    logger->log("Initializing MainAppWindow (UI Manager)...");
    if (!uiManager->init("Leap Motion App", 1280, 800)) {
    // Enable v-sync to cap GUI rendering to monitor refresh rate
    SDL_GL_SetSwapInterval(1);
        logger->log("ERROR: Failed to initialize MainAppWindow (SDL window)!");
        MessageBoxW(NULL, L"Failed to create main window!", L"Window Error", MB_ICONEXCLAMATION | MB_OK);
        // Clean up tray icon etc. on failure
        Shell_NotifyIconW(NIM_DELETE, &g_notifyIconDataW);
        if (g_notifyIconDataW.hIcon) DestroyIcon(g_notifyIconDataW.hIcon);
        DestroyWindow(g_hWndHidden);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    logger->log("MainAppWindow (SDL window) initialized (should be hidden).");

    // +++ Inject Controllers into MainAppWindow +++
    auto* oscController = appCorePtr->getOscController(); 
    if (!oscController) {
        logger->log("FATAL: OscController pointer is null after AppCore construction!");
        MessageBoxW(NULL, L"OscController pointer is null after AppCore construction!", L"Startup Error", MB_ICONSTOP | MB_OK);
        // Perform cleanup similar to other fatal errors
        Shell_NotifyIconW(NIM_DELETE, &g_notifyIconDataW);
        if (g_notifyIconDataW.hIcon) DestroyIcon(g_notifyIconDataW.hIcon);
        DestroyWindow(g_hWndHidden);
        return 1;
    }
    // Perform dynamic_cast for configManager
    auto* configInterface = dynamic_cast<ConfigManagerInterface*>(configManager.get());
    if (!configInterface) {
        logger->log("FATAL: Failed to cast ConfigManager to ConfigManagerInterface!");
        MessageBoxW(NULL, L"Failed to cast ConfigManager!", L"Startup Error", MB_ICONSTOP | MB_OK);
        // Perform cleanup similar to other fatal errors
        Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
        if (g_notifyIconData.hIcon) DestroyIcon(g_notifyIconData.hIcon);
        DestroyWindow(g_hWndHidden);
        return 1;
    }
    uiManager->setControllers(configInterface, oscController); // Pass casted pointer
    logger->log("ConfigManager and OscController injected into MainAppWindow.");
    // +++ End Injection +++

    // --- Start the main application core logic ---
    if (appCorePtr) {
        logger->log("Starting AppCore...");
        appCorePtr->start();
        logger->log("AppCore started.");

        // --- Get and log the main window handle AFTER init --- 
        g_hWndMain = uiManager->getHWND(); // Assign to global
        if (g_hWndMain) {
            logger->log("MainAppWindow::getHWND() returned valid handle after init: " + std::to_string(reinterpret_cast<uintptr_t>(g_hWndMain)));
        } else {
            logger->log("ERROR: MainAppWindow::getHWND() returned NULL after init!");
            MessageBoxW(NULL, L"Failed to get main window handle after initialization!", L"Window Handle Error", MB_ICONEXCLAMATION | MB_OK);
            // Clean up console before exiting
            // if (fpOut) fclose(fpOut);
            // if (fpErr) fclose(fpOut);
            // FreeConsole();
            return 1; // Exit if handle is invalid
        }

        // Explicitly set the window state flag to false *before* the first toggle call
        g_isMainWindowVisible = false;

        // --- Perform initial main window visibility toggle --- (REMOVED - Starting minimized)
        logger->log("Main window initialized but kept hidden. App running in system tray.");
        // No need to check g_hWndHidden here; ToggleMainWindowVisibility handles showing the main window (g_hWndMain)
        // Pass g_hWndHidden only for potential error message boxes *inside* the function.
        // ToggleMainWindowVisibility(g_hWndHidden, locator); // REMOVED: Don't show window initially

    } else {
        logger->log("CRITICAL ERROR: Cannot start AppCore, pointer is null!");
        // Handle error appropriately, maybe exit?
        return 1;
    }

    // --- SDL Event/Render Loop --- CORRECTED STRUCTURE ---
    SDL_Event event;
    bool running = true;
    while (running) { // Main application loop
        // 1. Poll for events
        while (SDL_PollEvent(&event)) { // Process all pending events this frame
            if (event.type == SDL_QUIT) {
                logger->log("SDL_QUIT event received.");
                running = false;
                break; // Exit inner SDL_PollEvent loop
            }
            // Pass event to ImGui etc.
             if (uiManager) { 
                 uiManager->processEvent(&event);
             }
        }
        if (!running) break; // Exit outer while(running) loop if SDL_QUIT occurred

        // 2. Process application logic (e.g., Leap frames)
        if (appCorePtr) {
            int framesProcessed = appCorePtr->processPendingFrames();
            // Optional: Log frames processed if needed
            // if (framesProcessed > 0) { logger->log(...); }

            // +++ FIX: Add sleep if no frames were processed to prevent busy-loop +++
            if (framesProcessed == 0) {
                // Only sleep briefly if there was nothing to do, 
                // but don't sleep if the window is hidden (already handled below)
                if (g_isMainWindowVisible) {
                   std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
                }
            }
        }

        // 3. Render UI (if window is visible)
        if (g_isMainWindowVisible && g_hWndMain && uiManager) {
             // logger->log("Rendering UI frame..."); // Uncomment if needed
             uiManager->render(); // Calls ImGui render, glClear, SwapWindow etc.
        } else {
            // Window is hidden, prevent busy-waiting
             SDL_Delay(10); // Sleep for ~10ms
        }
    } // End of main application loop
    // --- End SDL Event/Render Loop ---

    logger->log("Exited main application loop.");

    // --- Robust Shutdown Sequence ---
    // 1. Stop AppCore (joins LeapInput thread)
    if (appCorePtr) {
        logger->log("Stopping AppCore before UI shutdown...");
        appCorePtr->stop();
    }
    // 2. Shutdown UI (calls SDL_Quit)
    if (uiManager) {
        logger->log("Shutting down UIManager...");
        uiManager->shutdown(); // Ensure this calls MainAppWindow::shutdown() and SDL_Quit
    }

    logger->log("WinMain finished.");
    return 0; // Standard exit code after loop termination
}

// UI Management Function - Refactored to ONLY toggle visibility
void ToggleMainWindowVisibility(HWND hWndHidden, ServiceLocator& locator) {
    std::shared_ptr<AppLogger> logger;
    // Removed MainAppWindow retrieval as it's not needed for just showing/hiding
    try {
        logger = locator.get<AppLogger>();
    } catch (const std::exception& e) {
        std::string errorMsg = "Failed to get logger for ToggleMainWindowVisibility: ";
        errorMsg += e.what();
        ::MessageBoxW(hWndHidden, std::wstring(errorMsg.begin(), errorMsg.end()).c_str(), L"Internal Error", MB_ICONERROR | MB_OK);
        return; // Cannot proceed without logger
    }

    // Check if the main window handle is valid
    if (!g_hWndMain) {
        logger->log("ERROR: Cannot toggle main window visibility - g_hWndMain is NULL.");
        ::MessageBoxW(hWndHidden, L"Cannot toggle main window visibility - main window handle is missing.", L"Error", MB_ICONWARNING | MB_OK);
        // Resetting the flag might be appropriate if the handle is lost
        g_isMainWindowVisible = false; 
        return;
    }

    // Toggle visibility based on the current state flag
    if (g_isMainWindowVisible) { // If flag says it should be visible, hide it
        logger->log("Hiding Main Window...");
        ShowWindow(g_hWndMain, SW_HIDE);
        g_isMainWindowVisible = false; // Update flag
    } else { // If flag says it should be hidden, show it
        logger->log("Showing Main Window...");
        ShowWindow(g_hWndMain, SW_SHOW);
        SetForegroundWindow(g_hWndMain); // Bring to front
        g_isMainWindowVisible = true; // Update flag
    }
}

// Tray Icon Context Menu Function - Modified to accept locator indirectly
void ShowContextMenu(HWND hWnd) { // hWnd here is g_hWndHidden
    // Retrieve the locator stored in the window's user data
    ServiceLocator* locatorPtr = reinterpret_cast<ServiceLocator*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (!locatorPtr) {
        ::MessageBoxW(hWnd, L"Internal Error: Cannot retrieve application state for context menu.", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
    ServiceLocator& locator = *locatorPtr; // Use reference
    std::shared_ptr<AppLogger> logger;
    try {
         logger = locator.get<AppLogger>();
    } catch (const std::exception& e) {
         // Corrected newline in constant error
         { std::string msg = "Failed to get logger for context menu: " + std::string(e.what()); ::MessageBoxW(hWnd, std::wstring(msg.begin(), msg.end()).c_str(), L"Internal Error", MB_ICONERROR | MB_OK); }
         return;
    }

    POINT pt;
    GetCursorPos(&pt);
    // Create menu using the RAII wrapper
    MenuHandle menuHandle(CreatePopupMenu());

    if (menuHandle) { // Check if creation succeeded
        // Determine "Open" or "Hide" based on current visibility (using global g_hWndMain)
        const WCHAR* openText = L"Open";
        if (g_isMainWindowVisible && g_hWndMain && IsWindowVisible(g_hWndMain)) {
            openText = L"Hide";
        }

        // Use .get() to pass the raw handle to WinAPI functions
        AppendMenuW(menuHandle.get(), MF_STRING, IDM_OPEN, openText);
        AppendMenuW(menuHandle.get(), MF_STRING, IDM_EXIT, L"Exit");

        // SetForegroundWindow is essential according to docs
        SetForegroundWindow(hWnd);

        // TrackPopupMenu blocks until a selection is made or the menu is dismissed
        TrackPopupMenu(menuHandle.get(), TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

        // PostMessage might be needed if TrackPopupMenu interferes with message loop
        // PostMessage(hWnd, WM_NULL, 0, 0);

        // No need to call DestroyMenu, menuHandle's destructor takes care of it.
        // DestroyMenu(hMenu); // Removed
    } else {
        logger->log("ERROR: Failed to create context menu.");
    }
}

// Hidden Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    ServiceLocator* locatorPtr = nullptr;

    // Special handling for WM_NCCREATE or WM_CREATE to store the pointer
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        locatorPtr = reinterpret_cast<ServiceLocator*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(locatorPtr));
    } else {
        // Retrieve the pointer for other messages
        locatorPtr = reinterpret_cast<ServiceLocator*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    // If locatorPtr is still null for relevant messages, something went wrong
    if (!locatorPtr && message != WM_NCCREATE && message != WM_CREATE && message != WM_DESTROY) {
        // OutputDebugStringA("WndProc Error: ServiceLocator pointer is null!\n");
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    // Now safely use locatorPtr where needed (dereference carefully or check)
    ServiceLocator& locator = *locatorPtr; // Assuming it's valid based on above check for relevant messages
    std::shared_ptr<AppLogger> logger; // Get logger inside relevant cases


    switch (message) {
    // Handle WM_CREATE as well, just in case WM_NCCREATE isn't sufficient (though usually is)
    case WM_CREATE:
        {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            locatorPtr = reinterpret_cast<ServiceLocator*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(locatorPtr));
        }
        break; // Important: break here, default processing might still be needed

    case WM_APP_TRAYMSG:
        if (!locatorPtr) break; // Need locator for logging/actions
        logger = locator.get<AppLogger>(); // Get logger for this message
        switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
                logger->log("Tray icon left-clicked.");
                ToggleMainWindowVisibility(hWnd, locator); // Pass locator
                break;
            case WM_RBUTTONUP:
                 logger->log("Tray icon right-clicked.");
                ShowContextMenu(hWnd); // ShowContextMenu now retrieves locator itself
                break;
        }
        break; // Handle this message completely

    case WM_COMMAND:
        {
            if (!locatorPtr) break; // Need locator for logging/actions
            logger = locator.get<AppLogger>(); // Get logger for this message
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case IDM_OPEN:
                    logger->log("Menu item 'Open/Hide' selected.");
                    ToggleMainWindowVisibility(hWnd, locator); // Pass locator
                    break;
                case IDM_EXIT:
                    logger->log("Menu item 'Exit' selected. Initiating shutdown...");
                    DestroyWindow(hWnd); // Triggers WM_DESTROY
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam); // Let default handle unknown commands
            }
        }
        break; // Handle relevant commands completely

    case WM_DESTROY:
         if (locatorPtr) {
             try {
                logger = locatorPtr->get<AppLogger>();
                logger->log("WM_DESTROY received for hidden window. Posting Quit message.");
             } catch (...) { /* Ignore logging failure during shutdown */ }
         } else {
            // Corrected newline in constant error
            OutputDebugStringA("WM_DESTROY: ServiceLocator pointer already null.\n");
         }
        Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData); // Clean up tray icon
        PostQuitMessage(0); // Signal application to exit main loop
        // Also post SDL_QUIT so SDL event loop exits
        SDL_Event quitEvent;
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
        break; // Handle this message completely

    default:
        // For all other messages, use the default window procedure
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0; // Indicate message was handled (for cases that don't return DefWindowProc)
}
