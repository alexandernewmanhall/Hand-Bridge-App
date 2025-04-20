# LeapApp Project Overview

---

## Project Overview

LeapApp is a modular hand tracking application using Leap Motion. It processes hand and finger data in real time and sends OSC messages. The application is built with CMake, uses GLM (math), SDL2 (windowing), ImGui (UI), and is fully unit tested with GTest/Catch2.

---

## Dependencies
- **Leap Motion SDK**: Place in `../LeapSDK` relative to project root, with `include` and `lib/x64` folders.
- **CMake**: Used for build configuration.
- **GLM**: Fetched via CMake FetchContent, placed in `build/_deps/glm-src/glm`.
- **SDL2**: Fetched via CMake FetchContent.
- **ImGui**: Included for UI rendering.
- **GTest/Catch2**: For unit testing (fetched via CMake).

---

## Features

*   **Real-time Hand Tracking:** Utilizes the Ultraleap Leap Motion SDK (v6 compatible) for low-latency hand and finger tracking.
*   **Multi-Device Support:** Connect and track multiple Leap Motion devices simultaneously.
*   **Configurable OSC Output:** Sends detailed tracking data (palm, wrist, fingers, pinch/grab strength) via Open Sound Control (OSC) messages.
    *   Target IP address and port are configurable.
    *   Data points (palm, wrist, individual fingers, etc.) can be individually enabled/disabled via `config.json`.
*   **Device Alias Management:** Assigns short, persistent aliases (e.g., `dev1`, `dev2`) to device serial numbers for user-friendly OSC addresses and configuration.
*   **Hand Assignment:** Allows assigning devices to logical "LEFT" or "RIGHT" hands (multiple devices can share the same assignment).
*   **System Tray Integration (Windows):** Runs minimized in the system tray with options to open/hide the control panel or exit.
*   **Configuration File:** Persists settings (OSC target, filters, aliases, hand assignments) in `config.json` located in `%LOCALAPPDATA%\Hand Bridge\`.
*   **Graphical User Interface:** Provides a control panel (built with SDL2, OpenGL, and ImGui) to monitor devices, configure settings (OSC Target, Filters, Hand Assignments), and view status messages.
*   **Modular Architecture:** Uses dependency injection, interfaces, and distinct pipeline stages for maintainability and testability.
*   **Unit Tests:** Includes unit tests (Google Test) for core components.
*   **Zero-Burst on Hand Loss:** Sends a burst of OSC messages setting all data points for an assigned hand to zero when tracking for that hand is lost, ensuring downstream systems register the absence.

---

## Configuration (`config.json`)

The application saves its settings to `config.json` in `%LOCALAPPDATA%\Hand Bridge\`.

**Example Structure:**

```json
{
    "booleanSettings": {
        "sendFingerIsExtended": false,
        "sendGrabStrength": true,
        "sendIndex": true,
        "sendMiddle": true,
        "sendPalm": true,
        "sendPalmNormal": false,
        "sendPalmOrientation": false,
        "sendPalmVelocity": false,
        "sendPinchStrength": true,
        "sendPinky": true,
        "sendRing": true,
        "sendThumb": true,
        "sendVisibleTime": false,
        "sendWrist": true
    },
    "device_aliases": {
        "LPM224300789": "dev1",
        "LPM224300999": "dev2"
    },
    "hand_assignments": {
        "LPM224300789": "LEFT",
        "LPM224300999": "RIGHT"
    },
    "low_latency_mode": false,
    "osc_ip": "127.0.0.1",
    "osc_port": 7000
}
```

**Key Sections:**

*   **OSC Settings:**
    *   `osc_ip`: (String) Target IP address for OSC messages.
    *   `osc_port`: (Integer) Target port for OSC messages.
*   **Filters:**
    *   `booleanSettings`: (Object) Contains boolean flags for enabling/disabling specific OSC data points (e.g., `sendPalm`, `sendThumb`, `sendPinchStrength`).
*   **Device Management:**
    *   `device_aliases`: (Object) Maps device serial numbers (keys) to short aliases (values, e.g., "dev1").
    *   `hand_assignments`: (Object) Maps device serial numbers (keys) to default hand assignments (values: "LEFT", "RIGHT", or omitted/empty for "None").
*   **Other:**
    *   `low_latency_mode`: (Boolean) Flag for low latency mode (currently informational).

---

## Numbered Pipeline (Architecture Section)

This section outlines the core data flow through the application's distinct processing stages:

- **00_Entrypoint (`main.cpp`)**: Initializes services (Logging, Config, UI), sets up the `ServiceLocator`, creates the `AppCore`, and runs the main message loop.
- **01_LeapPoller**: Manages the connection to the Leap Motion service, polls for device and tracking events, and forwards them.
- **02_LeapSorter**: Receives events from `LeapPoller`. Sorts tracking frames (`FrameData`) by device serial number and forwards them.
- **03_DataProcessor**: Processes `FrameData` received from `LeapSorter`.
    *   Retrieves the assigned hand ("LEFT"/"RIGHT") for the device using the `ConfigManager`.
    *   Filters the hand data (palm, wrist, fingers, orientation, velocity, etc.) based on the boolean settings loaded from `config.json`.
    *   Sends the filtered, raw tracking data (in millimeters) via OSC messages to the configured IP and port. OSC addresses are structured like `/leap/{alias}/{hand}/{dataType}` (e.g., `/leap/dev1/LEFT/palm/position`).
    *   Implements the "burst-zero" logic: If an assigned hand that was previously tracked is no longer present in a frame, it sends a set of OSC messages explicitly setting all its associated data points to zero.
- **04_OscSender**: Receives OSC message bundles from `DataProcessor` and sends them over UDP using `oscpack`.

---

## Dependency Injection Strategy (May 2024)

### Overview
To improve modularity, testability, and reduce reliance on global variables, LeapApp now employs a Dependency Injection (DI) strategy centered around a simple `ServiceLocator` pattern, combined with constructor injection for core components like `AppCore`.

### Implementation Details

1.  **`ServiceLocator` (`src/di/ServiceLocator.hpp`)**:
    *   A lightweight container acting as a central registry for shared services.
    *   Uses `std::unordered_map<std::type_index, std::shared_ptr<void>>` for type erasure, allowing registration and retrieval of services by their type.
    *   Provides `add<T>(std::shared_ptr<T>)` to register services and `get<T>()` to retrieve them. The actual implementation in `src/di/ServiceLocator.hpp` correctly handles registration, lookup, and type casting.
    *   Throws `std::runtime_error` if a service type is not found during retrieval or if a type is registered twice.
    *   Uses `std::shared_ptr` to manage the lifetime of registered services.

2.  **Composition Root (`WinMain` in `src/main.cpp`)**:
    *   The application entry point (`WinMain`) acts as the composition root.
    *   A `ServiceLocator` instance is created early in `WinMain`.
    *   Core shared services are instantiated using `std::make_shared`:
        *   `AppLogger`: A wrapper around `OutputDebugStringA` for injectable logging.
        *   `ConfigManager`: Handles loading/saving configuration (implements `ConfigManagerInterface`).
        *   `MainAppWindow`: The main UI window manager.
    *   These services are registered with the `ServiceLocator` instance using `locator.add(...)`.

3.  **Constructor Injection (`AppCore`)**:
    *   The `AppCore` constructor (`src/app/AppCore.hpp`, `.cpp`) was modified to accept its primary dependencies directly:
        *   `std::shared_ptr<ConfigManagerInterface>`
        *   `MainAppWindow&` (Passed by reference as its lifetime is managed by the locator)
        *   `std::shared_ptr<AppLogger>`
    *   Inside `WinMain`, these dependencies are retrieved from the `ServiceLocator` using `locator.get<T>()` and passed to the `AppCore` constructor when it's created (using `std::make_unique<AppCore>(...)`).
    *   This makes `AppCore`'s dependencies explicit and improves testability.

4.  **Removal of Globals**:
    *   Global pointers like `g_uiManagerPtr`, `g_appCorePtr`, and the global `g_logger` lambda have been removed from `main.cpp`.

5.  **Access in Win32 Callbacks (`WndProc`, etc.)**:
    *   Functions requiring access to services outside the main `WinMain` scope (like the hidden window procedure `WndProc` or helper functions like `ToggleMainWindowVisibility`, `ShowContextMenu`) retrieve the `ServiceLocator` instance.
    *   The pointer to the `locator` is stored as user data associated with the hidden window (`g_hWndHidden`) using `SetWindowLongPtr(hWnd, GWLP_USERDATA, ...)` during window creation (`WM_NCCREATE`/`WM_CREATE`).
    *   Callbacks retrieve this pointer using `GetWindowLongPtr(hWnd, GWLP_USERDATA)` and cast it back to `ServiceLocator*`.
    *   Services like `AppLogger` or `MainAppWindow` are then obtained via `locator->get<T>()` within these callbacks.

### Example Snippet (Conceptual Composition in `WinMain`)**
This snippet illustrates the *pattern* of service registration and dependency injection in `WinMain`. It is simplified for clarity and does not represent the exact code.
```cpp
// In WinMain()
ServiceLocator locator;
std::unique_ptr<AppCore> appCorePtr;

try {
    // --- Service Registration ---
    // Create and register shared services using the locator's `add` method.
    // The locator manages the lifetime of these shared_ptrs.
    auto logger = std::make_shared<AppLogger>();
    locator.add(logger); // Registers AppLogger

    auto configManager = std::make_shared<ConfigManager>();
    // Register under the interface type for abstraction
    locator.add<ConfigManagerInterface>(configManager);

    // Example: Assuming MainAppWindow constructor takes necessary args (...)
    auto uiManager = std::make_shared<MainAppWindow>(/* ... */);
    locator.add(uiManager); // Registers MainAppWindow

    // --- Dependency Injection into AppCore ---
    // Retrieve registered services using the locator's `get` method
    // and pass them to AppCore's constructor.
    appCorePtr = std::make_unique<AppCore>(
        locator.get<ConfigManagerInterface>(), // Get the ConfigManager via its interface
        *locator.get<MainAppWindow>(),         // Get MainAppWindow and pass by reference
        locator.get<AppLogger>()               // Get the logger
    );

    // Service registration and AppCore construction complete.
    // Any errors during add/get (e.g., double registration, service not found)
    // would have thrown std::runtime_error here.

} catch (const std::runtime_error& e) {
    // Log or display the error from locator add/get or AppCore construction
    OutputDebugStringA(("DI Setup Error: " + std::string(e.what()) + "\n").c_str());
    // Handle initialization failure (e.g., return error code)
    return 1;
} catch (...) {
    OutputDebugStringA("Unknown DI Setup Error\n");
    return 1; // Handle unexpected errors
}

// --- Window Creation & Locator Association ---
// Create the hidden window used for tray icon messages.
// Pass the address of the *local* locator instance as user data.
// This allows callbacks (like WndProc) to retrieve the locator later.
hWndHidden = CreateWindowEx(
    0,
    CLASS_NAME, // Assuming CLASS_NAME is defined for the hidden window class
    L"LeapAppHiddenWindow",
    0, 0, 0, 0, 0,
    HWND_MESSAGE, // Message-only window
    nullptr,
    hInstance,    // hInstance from WinMain parameters
    &locator      // Pass pointer to the locator instance
);

if (!hWndHidden) {
     OutputDebugStringA("Failed to create hidden window!\n");
     return 1; // Handle window creation failure
}

// --- Application Start & Message Loop ---
if (appCorePtr) {
    appCorePtr->start(); // Start the main application logic
}

// Standard Windows message loop
MSG msg = {};
while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    // Integrate SDL/ImGui rendering if UI is visible (Conceptual)
    // if (uiManager && uiManager->isWindowVisible()) { // Check if UI should render
    //    SDL_Event event;
    //    while (SDL_PollEvent(&event)) { /* Handle SDL events */ }
    //    uiManager->render();
    // } else {
    //    Sleep(1); // Yield CPU if UI is hidden
    // }
}

// --- Application Stop ---
if (appCorePtr) {
    appCorePtr->stop(); // Cleanly shut down AppCore and its resources
}

// appCorePtr and locator go out of scope here.
// Services held by shared_ptr in the locator are automatically released.
// RAII handles (like IconHandle) also clean up automatically.

return (int)msg.wParam; // Return exit code from PostQuitMessage
```

### Rationale
- **Decoupling**: Reduces direct dependencies between components.
- **Testability**: Core components like `AppCore` can be tested more easily by injecting mock dependencies.
- **Clarity**: Makes dependencies explicit through constructor signatures.
- **Maintainability**: Easier to manage object lifetimes and understand service usage.
- **Removes Globals**: Eliminates issues associated with global state.

### Future Considerations & Advice
- **Pure Constructor Injection**: While the `ServiceLocator` helps, passing the locator itself into many classes can obscure dependencies. A future refactor could aim for *pure* constructor injection, where classes only receive the specific dependencies they need, not the locator. The locator would only be used at the top level (`WinMain`) to assemble the object graph.
- **Interfaces**: Introduce interfaces for more services (e.g., `IMainAppWindow`) to allow injecting mocks for UI testing and further decouple `AppCore` from concrete UI implementations.
- **DI Frameworks**: If the application's complexity grows significantly, consider adopting a more feature-rich C++ DI framework (e.g., Boost.DI, fruit, Miro.DI) which can automate parts of the object graph creation and provide more advanced features like lifecycle management and scopes.
- **Initialization Order**: Be mindful of initialization order, especially for services that depend on each other during construction. Initializing complex objects in the constructor body (as done for `DataProcessor` in `AppCore`) can sometimes be necessary if dependencies aren't ready during the member initializer list phase.

---

## System Tray Integration (Windows Only)

As of May 2024, LeapApp integrates with the Windows system tray (notification area) for background operation and easy access.

### Behavior
- On launch, the application starts minimized to the system tray without showing the main UI window.
- A LeapApp icon appears in the system tray (using `assets/dexteraICO.ico`).
- The core Leap Motion polling and OSC processing (`AppCore`, pipeline stages) start immediately in the background.
- Left-clicking the tray icon will show the main `MainAppWindow` (SDL/ImGui control panel) if hidden, or hide it if already visible. If the window hasn't been created and initialized yet, the `init()` method is called at this point (ensuring only a single initialization).
- Right-clicking the tray icon displays a context menu with options:
    - **Open/Hide**: Toggles the visibility of the main control panel window. The text changes dynamically based on the window's current state. If the window is not yet initialized, this option will create and show it.
    - **Exit**: Shuts down the application cleanly, stops Leap polling, saves the configuration (`config.json`), removes the tray icon, and terminates the process.
- Closing the main control panel window using the 'X' button (or the File->Exit menu) will trigger the `SDL_QUIT` event, which currently hides the window instead of terminating the loop. Use the tray menu's "Exit" option for complete shutdown.

### Implementation Details
- **Entry Point:** Uses the Windows-specific `WinMain` entry point in `main.cpp` instead of `main`.
- **Hidden Window:** A minimal, hidden WinAPI window (`g_hWndHidden` in `main.cpp`) is created at startup. This window owns the tray icon and receives messages related to it (`WM_APP_TRAYMSG`, defined in `main.cpp`).
- **Window Procedure (`WndProc` in `main.cpp`):** A dedicated `WndProc` handles messages for the hidden window:
    - `WM_APP_TRAYMSG`: Detects left (`WM_LBUTTONUP`) and right (`WM_RBUTTONUP`) clicks on the icon.
    - `WM_RBUTTONUP`: Calls `ShowContextMenu` to display the popup menu.
    - `WM_LBUTTONUP`: Calls `ToggleMainWindowVisibility`.
    - `WM_COMMAND`: Handles menu item selections (`IDM_OPEN`, `IDM_EXIT`, defined in `resource.h`). `IDM_OPEN` calls `ToggleMainWindowVisibility`, `IDM_EXIT` calls `DestroyWindow(g_hWndHidden)`.
    - `WM_DESTROY`: Triggered by `DestroyWindow`, this handler calls `Shell_NotifyIcon(NIM_DELETE, ...)` to remove the tray icon and `PostQuitMessage(0)` to terminate the application's message loop.
- **Tray Icon Management:** Uses `Shell_NotifyIcon` (`shell32.lib`) to add (`NIM_ADD`) and remove (`NIM_DELETE`) the icon. An icon resource (`IDI_APPICON`, defined in `resource.h`) referencing the `.ico` file in `resource.rc` is used.
- **Main Window (`MainAppWindow`):**
    - Initialization (`uiManager->init(...)`) is called *only once* within the `ToggleMainWindowVisibility` function (in `main.cpp`) when the flag `g_isUiInitialized` is false. This ensures SDL, OpenGL, and ImGui contexts are created just once when the window is first requested.
    - A `getHWND()` method was added to `MainAppWindow` (implemented in `.cpp` using `SDL_GetWindowWMInfo`) to retrieve the native window handle after initialization.
    - The `HWND` is needed by `WinMain` logic (specifically `ToggleMainWindowVisibility` in `main.cpp`) to show/hide the window using `ShowWindow()`.
    - The `SDL_QUIT` event (window 'X' button) is currently intercepted in the main loop in `main.cpp` to hide the window (`ShowWindow(..., SW_HIDE)`) instead of terminating the loop.
- **Message Loop Integration:** The main loop in `WinMain` is now a standard Windows message loop (`PeekMessage`, `TranslateMessage`, `DispatchMessage`).
    - When the main UI window is intended to be visible (`g_isUiInitialized && g_hWndMain && IsWindowVisible(g_hWndMain)`), the loop polls for SDL events (`SDL_PollEvent`) and calls `uiManager.render()`.
    - When the UI window is hidden or not yet initialized, `Sleep(1)` is used to yield CPU time.
- **Resource Files (`src/resource.h`, `src/resource.rc`):**
    - `resource.h`: Defines resource IDs (`IDI_APPICON`, `IDM_OPEN`, `IDM_EXIT`) using `#define` and standard include guards (`#ifndef`/`#define`/`#endif`).
    - `resource.rc`: Includes `<windows.h>` and `resource.h`, and defines the `ICON` resource, linking `IDI_APPICON` to the actual `.ico` file path.
    - **Note:** The Resource Compiler (`RC.exe`) can be sensitive to file encoding and line endings. Ensure `resource.h` is saved as **UTF-8 without BOM** with **CRLF** line endings and contains **exactly one trailing newline** after the `#endif` if encountering `RC1004` errors.
- **Build Configuration (`.vcxproj`):**
    - Includes `resource.h` in `<ClInclude>` items.
    - Compiles `resource.rc` using `<ResourceCompile>` item.
    - Links `shell32.lib` (added to `AdditionalDependencies`).
    - Sets the subsystem to `Windows` (`<SubSystem>Windows</SubSystem>`) to prevent a console window from appearing.
    - Adds specific `<ItemDefinitionGroup>` for `<ResourceCompile>` to set Culture and include paths if needed (though might not be strictly necessary if project structure is simple).

### Rationale
- Allows the application to run unobtrusively in the background, continuously processing Leap data and sending OSC messages.
- Provides a standard Windows user experience for accessing and exiting background applications.
- Decouples the core processing lifecycle (`AppCore`) from the main UI window's lifecycle, leveraging the existing separation of concerns.

---

## Recent Data Structure Updates (April 2025)

### HandData Structure
- `HandData` now uses `uint64_t visibleTime` instead of `float visibleTime` for precise time tracking, matching Leap SDK conventions and preventing data loss.
- `HandData` lives in `src/core/HandData.hpp` and is used throughout the pipeline for representing individual hand tracking data.

```cpp
struct HandData {
    std::string handType; // "left" or "right"
    PalmData palm;
    ArmData arm;
    std::vector<FingerData> fingers;
    float pinchStrength = 0;
    float grabStrength = 0;
    float confidence = 0;
    uint64_t visibleTime = 0; // Changed from float
    // ... other fields ...
};
```

### FrameData Structure (Unified)
- `FrameData` (in `src/core/FrameData.hpp`) is now the **single canonical structure** for all Leap Motion frame data throughout the application.

```cpp
struct FrameData {
    std::string deviceId;
    uint64_t timestamp = 0;
    std::vector<HandData> hands;
    // Extend with frameId or other metadata as needed
};
```

### Rationale for Changes
- **Single Source of Truth:** All frame-level data is now unified in `FrameData`, eliminating redundancy and confusion between raw/filtered/legacy types.
- **Type Safety:** Using `uint64_t` for time values prevents narrowing conversion warnings and matches the Leap Motion SDK.
- **Modularity & Clean Architecture:** All event types and data structures are in their own headers, supporting the pipeline-centric, dependency-injected architecture.

---

## Multi-Device Support (Leap Motion SDK v6)

LeapApp now supports robust multi-device tracking using the Leap Motion SDK v6, following the best practices demonstrated in the official MultiDeviceSample.

### Hand Assignment Logic (April 2025 Update)
- Multiple Leap Motion devices can now be assigned the same logical hand type ("LEFT" or "RIGHT") simultaneously.
- There is no longer any exclusivity enforcement: for example, several independent tracking stations can each assign their device as "LEFT".
- Downstream systems should use device aliases (included in OSC messages) to distinguish between devices with the same hand assignment.
- Assigning a hand type to one device does not affect any other device's assignments.

---

## Device Alias Lookup Strategy (April 2025)

### Overview

The UI device panel now fetches device aliases dynamically at render time, ensuring that alias information is always up-to-date and consistent with the application's configuration. The alias for a device serial number is never stored or assigned by the UI; instead, the UI queries the source of truth (DeviceAliasManager) via a read-only lookup.

### How It Works

- **Source of Truth:** `DeviceAliasManager` (owned by `ConfigManager`) maintains all device serial-to-alias mappings.
- **Read-Only Access:** The UI (MainAppWindow) does not store or assign aliases. It only reads them when rendering the device table.
- **Dependency Injection:** `AppCore` injects a lookup lambda into `MainAppWindow` during initialization:

---

## Device Table Sorting by Alias (April 2025)

### Overview

The device table in the UI is now sorted by the device alias (e.g., dev1, dev2, dev10) rather than the serial number. This ensures that devices are displayed in a user-friendly, natural order, matching the "devN" pattern commonly used in OSC output and configuration.

### Implementation Strategy

- **Temporary Row Structure:**
  - Inside `MainAppWindow::renderDevicePanel`, a local struct `DeviceRowDisplayData` is defined to hold the serial, alias, and a pointer to the per-device data.

- **Data Collection:**
  - The function iterates over `deviceTrackingDataMap` (protected by a mutex), fetching the alias for each serial using the injected lookup function.
  - Each device's info is pushed into a `std::vector<DeviceRowDisplayData>`.

- **Sorting:**
  - The vector is sorted using a custom lambda that implements natural sorting for the "devN" pattern (so `dev2` comes before `dev10`).
  - Devices with aliases not matching the pattern are sorted alphabetically after all `devN` entries.

- **Rendering:**
  - The ImGui table is rendered by iterating over the sorted vector, displaying the serial, alias, status, hand assignment, and other device data in each row.

### Example Code Snippet

```cpp
struct DeviceRowDisplayData {
    std::string serial;
    std::string alias;
    const PerDeviceTrackingData* deviceData;
};
std::vector<DeviceRowDisplayData> displayList;
for (const auto& [serial, data] : deviceTrackingDataMap) {
    std::string alias = aliasLookupFunc_ ? aliasLookupFunc_(serial) : "-";
    if (alias.empty()) alias = "-";
    displayList.push_back({serial, alias, &data});
}
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
// ... render table rows from displayList ...
```

### Rationale

- **Non-intrusive:** No changes to device connection or assignment logic; the sorting is purely for display.
- **User-Friendly:** Devices are shown in a logical, predictable order matching OSC and config conventions.
- **Maintainable:** The logic is confined to the UI rendering function, keeping the codebase clean and modular.

---

## RAII Resource Management Refactor (May 2024)

### Overview
To enhance resource safety, clarify ownership, and improve exception safety, the codebase underwent a systematic refactoring to enforce the Resource Acquisition Is Initialization (RAII) principle. This involved two main strategies: a generic wrapper for C-API handles and the consistent use of standard C++ smart pointers for dynamic memory.

### 1. C-API Handle Management with `HandleWrapper`

- **Generic RAII Wrapper (`src/utils/HandleWrapper.hpp`):**
  - A new template class `Handle<T, Deleter>` was introduced:
    ```cpp
    // In src/utils/HandleWrapper.hpp
    template<typename T, auto Deleter>
    class Handle {
    public:
        Handle() = default;
        explicit Handle(T h);
        ~Handle(); // Calls Deleter(h_) if h_ is valid
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        Handle(Handle&&) noexcept;
        Handle& operator=(Handle&&) noexcept;
        T get() const;
        T release();
        void reset(T h = T{});
        explicit operator bool() const;
    private:
        T h_ = T{};
    };
    ```
  - This wrapper ensures that any C-style handle assigned to it is automatically released via the specified `Deleter` function when the `Handle` object goes out of scope.

- **Specific Handle Types Defined:**
  - Type aliases were created using the `Handle` wrapper for specific WinAPI and LeapC handles:
    ```cpp
    // In src/core/LeapConnectionImpl.hpp
    inline void LeapDestroyConnectionWrapper(LEAP_CONNECTION conn) { /* ... */ }
    using LeapConnectionHandle = Handle<LEAP_CONNECTION, LeapDestroyConnectionWrapper>;

    // In src/main.cpp
    inline BOOL DestroyIconWrapper(HICON hIcon) { return DestroyIcon(hIcon); }
    inline BOOL DestroyMenuWrapper(HMENU hMenu) { return DestroyMenu(hMenu); }
    using IconHandle = Handle<HICON, DestroyIconWrapper>;
    using MenuHandle = Handle<HMENU, DestroyMenuWrapper>;
    ```

- **Application:**
  - **`LeapConnectionImpl` (`src/core/LeapConnectionImpl.cpp/.hpp`):** The raw `LEAP_CONNECTION connection_` member was replaced with `LeapConnectionHandle connectionHandle_`. Manual calls to `LeapDestroyConnection` in the destructor were removed.
  - **`main.cpp`:**
    - The global `HICON` for the system tray icon was replaced with the global `IconHandle g_trayIconHandle`. The manual `DestroyIcon` call at the end of `WinMain` was removed.
    - The temporary `HMENU` created within `ShowContextMenu` was replaced with a local `MenuHandle menuHandle`. The manual `DestroyMenu` call within that function was removed.

### 2. Standard Smart Pointers (`std::unique_ptr` / `std::shared_ptr`)

- **Strategy:** Systematically replace raw `new` allocations and corresponding `delete` calls with standard smart pointers to manage dynamically allocated memory.
  - `std::unique_ptr`: Used for objects with exclusive ownership (the most common case).
  - `std::shared_ptr`: Used primarily for services managed by the `ServiceLocator` where shared ownership is necessary.

- **Application / Verification:**
  - **`AppCore` (`src/app/AppCore.cpp/.hpp`):** Already used `std::unique_ptr` for pipeline stages (`leapInput_`, `dataProcessor_`, `oscSender_`, `uiController_`) and `std::shared_ptr` for injected services (`configManager_`, `logger_`). Verified no raw `new`/`delete`.
  - **`LeapInput` (`src/core/LeapInput.cpp/.hpp`):** Uses `std::unique_ptr<LeapPoller>`. Verified no raw `new`/`delete`.
  - **`OscSender` (`src/pipeline/04_OscSender.cpp/.hpp`):** The raw `UdpTransmitSocket* socket_` member was replaced with `std::unique_ptr<UdpTransmitSocket> socket_`. Manual `delete` calls in the destructor, `close`, and `initializeSocket` were removed/replaced with `socket_.reset()`.
  - **`MainAppWindow` (`src/ui/MainAppWindow.cpp/.h`):** The raw `SDL_Window* window` member was replaced with `std::unique_ptr<SDL_Window, SdlWindowDeleter> window_`, using a custom deleter struct (`SdlWindowDeleter`) to call `SDL_DestroyWindow`. The manual `SDL_DestroyWindow` call in `shutdown` was removed.
  - **`UIController` (`src/ui/UIController.cpp/.hpp`):** Managed via `std::unique_ptr` in `AppCore`. Verified no internal raw `new`/`delete`.
  - **Test Code (`tests/`):** A search confirmed no active use of raw `new`/`delete` in the current test files.

### 3. Object Pool Removal

- **Analysis:** A search located `src/utils/ObjectPool.h`, a custom object pool implementation.
- **Finding:** Further investigation revealed this pool was only referenced in files within the `backups/` directory, associated with legacy code (`LeapHandler.h`, `TrackingDataEvent`). It was not used by the active codebase.
- **Action:** The unused file `src/utils/ObjectPool.h` was deleted.

### Benefits
- **Resource Safety:** Automatic cleanup prevents leaks of C-API handles and dynamic memory.
- **Exception Safety:** Resources are correctly released even if errors occur and functions exit early.
- **Clear Ownership:** Smart pointers make object lifetime management explicit and reduce dangling pointer/double-delete errors.
- **Maintainability:** Code is cleaner, easier to understand, and safer to modify without manual resource tracking.

---

```cpp
// In AppCore
auto aliasLookup = [this](const std::string& serial) -> std::string {
    return configManager_.getDeviceAliasManager().lookupAlias(serial); // Read-only
};
uiManager_.setAliasLookupFunction(aliasLookup);
```

- **UI Integration:**
    - `MainAppWindow` stores this function and calls it in `renderDevicePanel` for each device row:
    ```cpp
    // In MainAppWindow.cpp
    std::string alias = "N/A";
    if (aliasLookupFunc_) {
        alias = aliasLookupFunc_(serial);
        if (alias.empty()) alias = "-";
    }
    ImGui::Text("%s", alias.c_str());
    ```
    - The "Alias" column is always up-to-date with the latest mapping from `DeviceAliasManager`.

### Rationale for Change

- **Consistency:** The alias is always fetched from the single source of truth, avoiding stale or inconsistent UI state.
- **Read-Only Guarantee:** The UI cannot accidentally assign or overwrite aliases.
- **Loose Coupling:** The lookup function is injected, so the UI does not depend directly on the internal structure of `ConfigManager` or `DeviceAliasManager`.
- **Thread Safety:** The lookup method is thread-safe for reads.

### DeviceAliasManager API

A new method was added for UI read-only lookup:
```cpp
// Returns the alias for a serial, or empty string if none exists
std::string DeviceAliasManager::lookupAlias(const std::string& serial) const;
```

---

## Troubleshooting Multi-Device Tracking (Important!)

A common issue encountered during development was that only one Leap device would provide tracking data, even when multiple devices were physically connected and detected by the application.

**Symptom:** Logs show multiple devices connecting (e.g., via `handleDeviceEvent` in `01_LeapPoller`), but `handleTracking` only receives frames associated with one device's serial number.

**Root Causes & Solutions:**

There are **two critical requirements** for enabling multi-device tracking with the LeapC API that must be correctly implemented:

1.  **Connection Must Be Multi-Device Aware:**
    *   **Problem:** The initial connection to the Leap service was created using default settings (`LeapCreateConnection(nullptr, ...)`), which does *not* enable multi-device mode.
    *   **Solution:** Modify the connection creation logic in `src/pipeline/00_LeapConnection.cpp` to explicitly configure the connection for multi-device awareness. Pass a `LEAP_CONNECTION_CONFIG` struct with the `eLeapConnectionConfig_MultiDeviceAware` flag set:
        ```cpp
        // In LeapConnection::LeapConnection() in 00_LeapConnection.cpp
        LEAP_CONNECTION_CONFIG config = {0};
        config.size = sizeof(config);
        config.flags = eLeapConnectionConfig_MultiDeviceAware;
        eLeapRS result = LeapCreateConnection(&config, &connection_); // Pass config!
        // ... handle result ...
        ```

## Unit Testing (April 2025)

LeapApp uses [Google Test](https://github.com/google/googletest) for unit testing. Unit tests are located in the `tests/` directory.

**How to run tests:**
1. Build the project using CMake with Google Test (see CMakeLists.txt for setup).
2. Run tests from the build directory:
   - `ctest -C Release --output-on-failure`
   - Or run the test executable directly: `./build/Release/test_DeviceAliasManager.exe`

### Current Unit Tests

---

## OSC Sender Integration & Testing (April 2025)

LeapApp uses the [oscpack](https://github.com/rossbencina/oscpack) library for Open Sound Control (OSC) message handling and UDP transmission. The `OscSender` class and its tests validate OSC message emission.

### oscpack Setup
- **Location:** oscpack source files are located in `../external/oscpack/` relative to the LeapApp project directory.
- **Required Files:** Ensure both `ip/UdpSocket.cpp`, `ip/NetworkingUtils.cpp`, and their corresponding headers are present. For Windows, use the files from `ip/win32/` in the oscpack distribution.
- **CMake Configuration:** The `CMakeLists.txt` for LeapApp explicitly lists all required oscpack `.cpp` files for the test targets. Paths are relative to the LeapApp directory (e.g., `../external/oscpack/ip/UdpSocket.cpp`).
- **Linking:** On Windows, `winmm` is linked to resolve `timeGetTime` dependencies in oscpack.

### Running the OSC Sender Tests
1. Build the project as usual (see above).
2. Run the OSC sender tests:
   - `ctest -C Release --output-on-failure -R OscSenderTest`
   - Or run the test executable directly: `./build/Release/test_OscSender.exe`

If the test passes, OSC message sending is correctly configured.

### Troubleshooting oscpack Build Errors
- **Missing UdpSocket.cpp / NetworkingUtils.cpp:**
  - Copy the appropriate files from your oscpack distribution (`ip/win32/` for Windows, `ip/posix/` for Linux/Mac) into `external/oscpack/ip/`.
- **Unresolved external symbol `__imp_timeGetTime`:**
  - Ensure `winmm` is linked in CMakeLists.txt (e.g., `target_link_libraries(test_OscSender PRIVATE ... winmm)`).
- **POSIX headers not found on Windows:**
  - You are likely using the POSIX versions of oscpack files. Replace them with the Windows versions from `ip/win32/`.

---

#### DataProcessor Tests (April 2025)
- **Tested Features:**
  - OSC message emission for palm, wrist, and individual fingers (thumb, index, etc.)
  - Special case: thumb with no bones emits `/thumb/exists` OSC message if thumb filter is enabled
  - Filtering logic for palm, wrist, and each finger independently

#### Hand Presence & Tracking Dot Tests (2025)
- `tests/test_DataProcessor_HandPresence.cpp` covers:
  - No hands present: `handPresent` is false, `palmNorm` resets to center
  - One hand present: `handPresent` is true, `palmNorm` is normalized
  - Multiple hands: `handPresent` is true
  - Hand appears/disappears: state toggles correctly
- To run: `./Debug/test_DataProcessor_HandPresence.exe` after building
  - Device aliasing in OSC addresses
- **Results:**
  - All tests pass as of April 2025
  - Edge case for thumb with no bones is robustly covered
  - Diagnostic output confirms correct OSC emission and callback invocation

**How to run:**
- After building, run `./build/Release/test_DataProcessor.exe` (or use `ctest`)
- Output will indicate pass/fail and print diagnostic info if issues occur

---

#### 1. DeviceAliasManager
**File:** `tests/test_DeviceAliasManager.cpp`
- **AssignsUniqueAliases:** Different serial numbers get unique aliases.
- **ReturnsSameAliasForSameSerial:** The same serial always gets the same alias.
- **ClearsAliases:** After clearing, the same serial gets the same alias.
**Results:**
```
[==========] Running 3 tests from 1 test suite.
[  PASSED  ] 3 tests.
```

#### 2. LeapPoller (Mocked Device Enumeration)
**File:** `tests/test_LeapPoller.cpp`
- **TracksMultipleDevices:** Simulates connecting two devices with different serials and checks both are tracked.
**Results:**
```
[==========] Running 1 test from 1 test suite.
[  PASSED  ] 1 test.
```

#### 3. LeapSorter
**File:** `tests/test_LeapSorter.cpp`
- **CallsCallbackForEachDevice:** Sends frames for two devices and checks the callback is called for both with correct data.
**Results:**
```
[==========] Running 1 test from 1 test suite.
[  PASSED  ] 1 test.
```

#### 4. DeviceTrackingModel (UI Logic Model)
**File:** `tests/test_DeviceTrackingModel.cpp`
- **ConnectAssignAndCount:** Connects two devices, assigns hands, sets hand/frame counts, and checks state and serial list.
**Results:**
```
[==========] Running 1 test from 1 test suite.
[  PASSED  ] 1 test.
```

**Want to add more tests?**
- Add a new file to `tests/` (e.g., `test_ConfigManager.cpp`, `test_DataProcessor.cpp`).
- Add the new test file and any required source files to your `CMakeLists.txt`.
- Use the Google Test `TEST` macro to define your test cases.

---

## Device Alias Management (April 2025)

LeapApp uses a dedicated `DeviceAliasManager` to provide robust, persistent, and user-friendly aliases for each Leap device serial number. This enables:
- **Short, readable OSC addresses** (e.g., `/leap/dev1/left/palm` instead of `/leap/longserial/left/palm`)
- **Stable mapping across sessions**: Aliases are persisted in the config JSON file (`deviceAliases` section) and reloaded on startup.
- **Single source of truth**: The `DeviceAliasManager` is dependency-injected into pipeline components (e.g., `DataProcessor`), ensuring all OSC output uses consistent aliases.
- **Separation of concerns**: Alias logic is separated from config, device polling, and OSC output logic.

### How it Works
- On device connect, the serial number is mapped to a short alias (e.g., `dev1`).
- If the serial is already known, its alias is reused.
- The mapping is saved and loaded via the config file (see below).
- All OSC addresses use the alias instead of the full serial number.

#### Example config section
```json
{
  "deviceAliases": {
    "AABBCCDDEEFF": "dev1",
    "112233445566": "dev2"
  }
}
```

#### Example OSC address
```
/leap/dev1/left/palm
```

### Pipeline Integration
- The `DeviceAliasManager` is owned by `ConfigManager` and injected into `DataProcessor`.
- `DataProcessor` uses the alias when constructing OSC addresses.
- On save/load, aliases are persisted alongside other config settings.

---

### Key Concepts
- **Device Identification:**
  - Each Leap device is assigned a unique `device_id` (uint32_t) by the LeapC API.
  - Each device also has a serial number (string), obtained via `LeapGetDeviceInfo`.
- **Device Mapping:**
  - The application maintains a mapping of `device_id` to device handle and serial number (`DeviceInfo` struct in `01_LeapPoller.hpp`).
  - This mapping is updated on device connect/disconnect events.
- **Tracking Event Routing:**
  - Each tracking event (`eLeapEventType_Tracking`) includes a `msg.device_id` (not in the tracking struct, but in the Leap connection message).
  - The app uses `msg.device_id` to look up the correct serial number and device handle for processing and file naming.

### Requirements for Multi-Device Support
- **Leap Motion SDK v6** (or later) must be installed and available on your system.
- **Correct Device Event Handling:**
  - Device connect/disconnect events must be handled to keep the mapping up to date.
- **Unique Serial Numbers:**
  - Serial numbers must be retrieved and used for file naming and UI display.
- **Pipeline Integration:**
  - All pipeline stages that consume tracking data must be aware of the device mapping and use serial numbers or device IDs as needed.

### Architectural Flow
1. **Device Connection:**
   - On `eLeapEventType_Device`, open the device, retrieve its `device_id` and serial number, and add to the mapping.
   - The serial number is assigned a short alias by `DeviceAliasManager` (if not already present).
2. **Device Disconnection:**
   - On `eLeapEventType_DeviceLost`, remove the device from the mapping and close its handle.
3. **Tracking Event:**
   - On `eLeapEventType_Tracking`, use `msg.device_id` to find the corresponding serial number and handle.
   - The alias for the serial number is used in all downstream OSC output.
   - Pass the serial number along with the tracking event to downstream pipeline stages.

#### Visual Flow Diagram
```mermaid
flowchart TD
    subgraph Leap Motion Service
        A[Leap Device Connects]
        B[Leap Device Disconnects]
        C[Tracking Event Received]
    end
    subgraph LeapApp Pipeline
        D[Device Event Handler\n(01_LeapPoller)]
        E[Device Mapping Table\n(device_id <-> serial)]
        H[DeviceAliasManager\n(serial <-> alias)]
        F[Tracking Event Handler\n(01_LeapPoller)]
        G[Downstream Pipeline\n(02_LeapSorter, 03_DataProcessor, etc.)]
    end

    A -- eLeapEventType_Device --> D
    D -- Add device_id/serial to --> E
    D -- Assign alias to serial --> H
    B -- eLeapEventType_DeviceLost --> D
    D -- Remove device_id/serial from --> E
    C -- eLeapEventType_Tracking --> F
    F -- Lookup serial by device_id --> E
    E -- Provide serial --> F
    F -- Lookup alias by serial --> H
    H -- Provide alias --> G
    F -- Pass tracking + alias --> G
```

### Relevant Files and Structures
- [`src/pipeline/01_LeapPoller.cpp`](./src/pipeline/01_LeapPoller.cpp): Implements device polling, mapping, and event routing.
- [`src/pipeline/01_LeapPoller.hpp`](./src/pipeline/01_LeapPoller.hpp): Defines `DeviceInfo` struct and device management logic.
- **LeapC API headers**: (`LeapC.h`) for device and event definitions.

### Best Practices and Maintenance
- Always use `msg.device_id` from the Leap connection message to identify the device for tracking events.
- Ensure serial numbers are used for any persistent data (e.g., file naming) to prevent ambiguity.
- When extending device handling, update both the mapping and event routing logic to ensure all devices are correctly tracked.
- Refer to the Leap Motion SDK's `MultiDeviceSample.c` for reference logic and troubleshooting.

---

## Modularization Strategy

LeapApp is designed with a strict modularization strategy to maximize maintainability, testability, and clarity:
- **Single Responsibility:** Each struct, event, or class is defined in its own dedicated header/source file whenever possible.
- **Event Modularization:** All event types (e.g., `DeviceConnectedEvent`, `DeviceLostEvent`, `DeviceHandAssignedEvent`) are defined in their own headers in `src/core/`, decoupled from legacy or monolithic files. This makes them easily reusable across the pipeline and UI without unwanted dependencies.
- **Pipeline Independence:** Each numbered pipeline stage, UI, and controller component includes only the types it needs, minimizing coupling and making the codebase easier to extend or refactor.
- **Legacy Decoupling:** As part of the refactor, event types and data structures are gradually extracted from legacy files (such as `LeapHandler.h`) and placed into their own headers. This supports a clean, pipeline-centric architecture.

---

**README UPDATE RULE:**
Whenever you make any change to the logic flow, data structures, event handling, or filtering strategies in the LeapApp, update this README to:
- Precisely describe the new or modified logic flows and their rationale
- Document the names, types, and purposes of any new or changed variables, data structures, or events
- Update or add detailed flow diagrams or step-by-step descriptions as needed
- Ensure that developers can understand not just what changed, but why and how it fits into the overall architecture
- Use technical, explicit language suitable for maintainers and future contributors

This document provides an in-depth overview of the LeapApp project architecture, focusing on its modular, numbered pipeline for Leap Motion tracking and OSC (Open Sound Control) output.

---

## Project Goals
- **Separation of Concerns**: Each pipeline stage is responsible for a single, well-defined task.
- **Dependency Injection**: Logic layers are connected via explicit callbacks, making testing and substitution easy.
- **Modularity**: Each numbered pipeline component can be developed, tested, or replaced independently.
- **GUI Integration**: The ImGui-based interface allows real-time control of device assignments and OSC output filtering. It receives device status updates (connected/disconnected/assigned hand) from the core application logic.
- **Configuration**: Settings (OSC endpoint, hand assignments, filters) are persisted in `%LOCALAPPDATA%\Hand Bridge\config.json`.

---

## Pipeline Architecture

LeapApp uses a numbered, modular pipeline for Leap Motion data flow:

```mermaid
flowchart TD
    subgraph UI
        MainAppWindow[MainAppWindow (UI View)]
        UIController[UIController (UI State/Logic)]
    end
    subgraph Pipeline
        Z[00_LeapConnection] --> A[01_LeapPoller] --> B[02_LeapSorter] --> C[03_DataProcessor] --> D[04_OscSender]
    end
    subgraph Core
        ConfigManager
        LeapInput[LeapInput (Polling Thread)]
        AppCore
    end
    subgraph External
        User[User Interaction]
        ConfigFile[config.json] --> ConfigManager
        OSCReceiver[OSC Receiver]
    end

    User --> MainAppWindow
    MainAppWindow -- "User actions (hand assign, filter toggles, gain/soft zone changes)" --> UIController // Updated actions
    UIController -- "Hand assign cmd" --> LeapSorter
    UIController -- "Filter/Gain/SoftZone changes" --> ConfigManager // Updated actions
    UIController -- "Filter/Gain/SoftZone changes callback" --> AppCore // Callback notifies AppCore
    AppCore -- "Update DataProcessor settings" --> DataProcessor // AppCore updates DataProcessor
    LeapInput -- "requests poll" --> A
    A -- "Raw tracking / Device Events" --> LeapInput
    LeapInput -- "Raw tracking data + serial" --> B
    LeapInput -- "Device Events" --> AppCore
    AppCore -- "Device Events" --> MainAppWindow
    AppCore -- "Loads Config" --> ConfigManager
    AppCore -- "Saves Config" --> ConfigManager
    B -- "FrameData" --> C # LeapSorter sends frame data to DataProcessor
    C -- "OSC messages" --> D
    C -- (No Connection) --> MainAppWindow # DataProcessor does NOT currently send TrackingDataEvents to UI
    D -- "Sends OSC packets" --> OSCReceiver
```

### Pipeline Stages
- **00_LeapConnection**: Manages the creation, opening, and destruction of the Leap Motion SDK connection. All pipeline stages depend on this as the foundational resource. Implemented with a .hpp/.cpp split for separation of concerns and easier dependency injection.
- **01_LeapPoller**: Handles Leap device enumeration and polling *requests*. Its `poll()` method performs a single call to `LeapPollConnection` and processes the resulting event (e.g., Tracking, Device Connect/Lost, Policy Change). It no longer manages its own polling thread or loop; that responsibility lies with `LeapInput`. Emits raw tracking events and device status events via callbacks. Handles `eLeapEventType_Connection`, `eLeapEventType_Policy`, `eLeapEventType_Device`, `eLeapEventType_DeviceLost`, `eLeapEventType_Tracking`, logging other/unknown types to `std::cerr`.
- **02_LeapSorter**: Receives events from `LeapPoller`. Sorts tracking frames (`FrameData`) by device serial number and forwards them.
- **03_DataProcessor**: Processes `FrameData` received from `LeapSorter`.
    *   Retrieves the assigned hand ("LEFT"/"RIGHT") for the device using the `ConfigManager`.
    *   Filters the hand data (palm, wrist, fingers, orientation, velocity, etc.) based on the boolean settings loaded from `config.json`.
    *   Sends the filtered, raw tracking data (in millimeters) via OSC messages to the configured IP and port. OSC addresses are structured like `/leap/{alias}/{hand}/{dataType}` (e.g., `/leap/dev1/LEFT/palm/position`).
    *   Implements the "burst-zero" logic: If an assigned hand that was previously tracked is no longer present in a frame, it sends a set of OSC messages explicitly setting all its associated data points to zero.
- **04_OscSender**: Receives OSC message bundles from `DataProcessor` and sends them over UDP using `oscpack`.

---

### UI and Logic
- **MainAppWindow**: (Formerly `UIManager`) Manages the main SDL window, OpenGL context, ImGui setup, and renders the overall UI layout using ImGui (Device Panel, OSC Settings Panel, etc.).
   *   Receives device status updates (connected, lost, hand assigned) via direct method calls from `AppCore` (`handleDeviceConnected`, `handleDeviceLost`, `handleDeviceHandAssigned`).
   *   Uses an injected `UIController` pointer to read current filter/OSC settings for display and to send user actions (button clicks for hand assignment, checkbox changes for filters) back to the `UIController`.
   *   **Note:** `MainAppWindow` currently *does not* receive live tracking data updates (like FPS, hand count, pinch/grab strength) from the pipeline. While it has a `handleTrackingData` method intended for this, that method is not currently called from anywhere in the application flow. The device panel displays data like FPS and hand counts based on its own internal state (`PerDeviceTrackingData`), which is *not* being updated by incoming tracking events.
- **UIController**: Bridges `MainAppWindow` and the pipeline/configuration (`LeapSorter`, `DataProcessor`, `ConfigManager`).
   *   Owns the active UI state (e.g., OSC IP/port buffers, current filter settings).
   *   Receives user actions from `MainAppWindow` (e.g., hand assignment button clicks, filter checkbox changes, OSC setting changes).
   *   Sends commands to `LeapSorter` (via `setDeviceHandAssignment`) and `DataProcessor` (via `setFilterState` which triggers an update callback to `AppCore` which calls `DataProcessor::setFilterSettings`).
   *   Communicates OSC IP/Port changes to `ConfigManager` and the live `OscSender` via callbacks configured in `AppCore`.
   *   Initializes its state from `ConfigManager` on startup.

## Application Lifecycle & System Tray Operation (April 2025)

This section describes how the LeapApp application starts, runs, and interacts with the user via the system tray.

### Startup Sequence (Minimized to Tray)

1.  **Initialization**: Core services (Logging, Config, UI), sets up the `ServiceLocator`, creates the `AppCore`, and runs the main message loop.
2.  **Hidden Window Creation**: A dedicated, invisible message-only window (`g_hWndHidden`) is created using the Win32 API. Its window procedure (`WndProc`) is responsible for handling messages related to the system tray icon. The `ServiceLocator` instance is associated with this window's user data for access within `WndProc`.
3.  **Tray Icon Registration**: The application icon (`IDI_APPICON`) is loaded and registered with the Windows system tray using `Shell_NotifyIcon`. Windows is instructed to send `WM_APP_TRAYMSG` messages to the hidden window upon user interaction with the icon.
4.  **Main Window Creation (Hidden)**: The main application window (`g_hWndMain`) is created using SDL (`SDL_CreateWindow`) but with the `SDL_WINDOW_HIDDEN` flag. This ensures the window exists and its rendering context (OpenGL, ImGui) is ready, but it does not appear on screen initially. The `g_isMainWindowVisible` flag is set to `false`.
5.  **AppCore Start**: The `AppCore`'s background threads (e.g., for Leap polling) are started.
6.  **Main Loop Entry**: The application enters the main `while(running)` loop in `WinMain`. The application is now running, minimized to the system tray.

### System Tray Interaction

User interaction with the tray icon is handled by `WndProc` (the message handler for `g_hWndHidden`):

-   **Left-Click**: Toggles the visibility of the main application window (`g_hWndMain`) using `ToggleMainWindowVisibility`, which calls `SDL_ShowWindow` or `SDL_HideWindow` and updates the `g_isMainWindowVisible` flag.
-   **Right-Click**: Displays a context menu (`ShowContextMenu`) with options:
    -   **Show/Hide**: Also calls `ToggleMainWindowVisibility`.
    -   **Exit**: Sends a `WM_CLOSE` message to the hidden window (`g_hWndHidden`) to initiate shutdown.

### Main Application Loop (Conditional Rendering)

The main `while(running)` loop in `WinMain` performs the following continuously:

1.  **Event Polling (`SDL_PollEvent`)**: Checks for and processes SDL events (e.g., `SDL_QUIT`).
2.  **Data Processing (`AppCore::processPendingFrames`)**: Processes incoming Leap Motion data.
3.  **Conditional Rendering**:
    -   If `g_isMainWindowVisible` is `true`, it calls `MainAppWindow::render()` to draw the ImGui UI to the main window.
    -   If `g_isMainWindowVisible` is `false`, it calls `SDL_Delay(10)` to yield CPU time and prevent busy-waiting.

### Shutdown Sequence

1.  **Initiation**: User selects "Exit" from the tray context menu, triggering `WM_CLOSE` on `g_hWndHidden`.
2.  **`WndProc` `WM_CLOSE`**: Calls `DestroyWindow(g_hWndHidden)`.
3.  **`WndProc` `WM_DESTROY`**: As the hidden window is destroyed, this handler:
    -   Removes the icon from the system tray (`Shell_NotifyIcon(NIM_DELETE, ...)`).
    -   Calls `PostQuitMessage(0)`.
4.  **`SDL_QUIT` Event**: `PostQuitMessage(0)` generates an `SDL_QUIT` event.
5.  **Main Loop Exit**: The main loop's `SDL_PollEvent` detects `SDL_QUIT`, sets `running = false`, and the loop terminates.
6.  **Cleanup**: Code after the main loop in `WinMain` executes, shutting down `AppCore`, saving configuration, and releasing resources before the application exits cleanly.

---

## Potential Optimizations

Based on code analysis, here are a couple of areas for potential improvement regarding code structure, testability, and build performance:

### 1. Improve Dependency Management (Service Locator vs. Constructor Injection)

*   **Issue:** The current use of a Service Locator (`ServiceLocator.hpp`) throughout the application, particularly in pipeline stages, can hide dependencies and make unit testing more complex.
    *   **Why (Problems with Service Locator):**
        *   **Hidden Dependencies:** It's not immediately clear from a class's interface (its header file/constructor) what external services it relies on. You need to read the implementation to find `ServiceLocator::get<T>()` calls.
        *   **Testability Challenges:** Unit testing classes that use a global service locator requires setting up and managing the state of the locator itself, often involving registration of mock/fake services within the test setup. This couples tests to the locator mechanism.
        *   **Tight Coupling:** Classes become coupled to the `ServiceLocator` itself, not just the services they need.
*   **Suggestion:** Keep the `ServiceLocator` for wiring the application together in `main` or `AppCore`, but modify classes (especially pipeline stages) to *also* have constructors that explicitly accept their dependencies (Dependency Injection).
    *   **How (Implementation Steps):**
        1.  For each class using the locator (e.g., `LeapPoller`), identify the services it retrieves (e.g., `AppLogger`, `IConfigStore`).
        2.  Add a new constructor (or modify the existing one) to accept these services as parameters (e.g., `LeapPoller(std::shared_ptr<AppLogger> logger, std::shared_ptr<IConfigStore> config)`).
        3.  Store these passed-in dependencies as member variables within the class.
        4.  Update the class's methods to use these member variables instead of calling `ServiceLocator::get<T>()`.
        5.  In unit tests, use the new constructor to pass mock/fake dependencies directly.
        6.  In the main application setup, continue using the `ServiceLocator` to retrieve services, but *then* pass them into the new constructors when creating objects.
    *   **Why (Benefits of Constructor Injection):**
        *   **Explicit Dependencies:** Dependencies are clearly visible in the constructor signature.
        *   **Improved Testability:** Tests can directly pass mocks to the constructor without needing to interact with the global locator.
        *   **Reduced Coupling:** Classes depend directly on the interfaces/types they need, not on the locator mechanism.

### 2. Refactor Header-Only Pipeline Implementation

*   **Issue:** Several core components, particularly the pipeline stages (`01_LeapPoller.hpp`, `02_LeapSorter.hpp`, etc.), appear to have their full implementation within the header files (`.hpp`).
    *   **Why (Problems with Header-Only):**
        *   **Increased Build Times:** Every `.cpp` file that includes a header-only implementation causes the compiler to re-process and re-compile that entire implementation. Changes to the header force recompilation of all including files, slowing down incremental builds significantly.
        *   **Larger Executable Size:** The compiled code for the header-only classes can be duplicated across multiple object files, potentially bloating the final executable.
        *   **Complex Template/Link Errors:** While less common for non-template code, header-only implementations (especially with `inline` or static data) can sometimes lead to harder-to-diagnose One Definition Rule (ODR) violations or obscure template instantiation errors.
*   **Suggestion:** Move the implementation details (method bodies) out of the header files (`.hpp`) and into corresponding source files (`.cpp`).
    *   **How (Implementation Steps):**
        1.  For each header-only class (e.g., `LeapPoller`), create a corresponding source file (e.g., `01_LeapPoller.cpp`).
        2.  In the `.hpp` file, keep only the class definition, member variable declarations, and method *declarations* (signatures ending with `;`). Remove method bodies (`{ ... }`).
        3.  Move the method bodies from the `.hpp` file to the new `.cpp` file.
        4.  In the `.cpp` file, prefix each method implementation with the class name and scope resolution operator (e.g., `void LeapPoller::poll() { /* ... */ }`).
        5.  Include the corresponding header at the top of the `.cpp` file (e.g., `#include "01_LeapPoller.hpp"`).
        6.  Ensure necessary includes for implementation details are present in the `.cpp` file. Reduce includes in the `.hpp` file to only those needed for the declaration (consider forward declarations).
        7.  Add the new `.cpp` file to the project's build system (e.g., `CMakeLists.txt` or `.vcxproj`).
    *   **Why (Benefits of Separate Implementation):**
        *   **Faster Compilation:** Implementations are compiled only once. Changes to a `.cpp` file only trigger recompilation of that file (and relinking), significantly speeding up incremental builds.
        *   **Reduced Build Dependencies:** `.cpp` files including the header are less likely to need recompilation when only the implementation (`.cpp`) changes.
        *   **Smaller Executable Size:** Code duplication is reduced.
        *   **Clearer Separation of Interface and Implementation:** Improves code organization and readability.

```
