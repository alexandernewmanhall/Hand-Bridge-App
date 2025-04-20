#include "AppCore.hpp"
#include "../core/interfaces/IFrameSource.hpp"
#include "../core/LeapInput.hpp" // Ensure LeapInput is visible for construction and casting
#include "../pipeline/00_LeapConnection.hpp"
#include "../core/DeviceHandAssignedEvent.hpp"
#include <sstream>
#include "../ui/MainAppWindow.h"
#include <iostream>
#include <string>
#include <vector>
#include "../core/Log.hpp"
#include "../core/HandData.hpp"
#include "../core/DeviceConnectedEvent.hpp"
#include "../core/DeviceLostEvent.hpp"
#include "../core/FrameData.hpp"
#include "../core/ConfigManager.h"
#include "../utils/SpscQueue.hpp" // Include the queue header
#include "../ui/UIController.hpp" // For HandAssignmentEvent

#include "../pipeline/01_LeapPoller.hpp"
#include "../pipeline/02_LeapSorter.hpp"
#include "../pipeline/03_DataProcessor.hpp"
#include "../pipeline/04_OscSender.hpp"
#include "../ui/UIController.hpp"
#include "../core/DeviceAliasManager.hpp"
#include "transport/osc/OscMessage.hpp"
#include "../core/AppLogger.hpp"

// Define queue capacity here or make it configurable
const size_t FRAME_QUEUE_CAPACITY = 256;

// Process queued hand assignments from UIController
void AppCore::processQueuedHandAssignments() {
    if (!uiController_) return;
    auto& queueMutex = uiController_->getEventQueueMutex();
    auto& queue = uiController_->getHandAssignmentQueue();
    std::vector<UIController::HandAssignmentEvent> queueCopy;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queueCopy.swap(queue);
    }
    for (const auto& event : queueCopy) {
        uiManager_.handleDeviceHandAssigned(DeviceHandAssignedEvent(event.serialNumber, DeviceHandAssignedEvent::stringToHandType(event.handType)));
    }
}
// Example: Call processQueuedHandAssignments() in your main loop after UI events are handled.
// appCore.processQueuedHandAssignments();

AppCore::AppCore(std::shared_ptr<IConfigStore> configManager,
                 MainAppWindow& uiManager,
                 std::shared_ptr<AppLogger> logger)
    : configManager_(std::move(configManager))
    , uiManager_(uiManager)
    , logger_(std::move(logger))
    // Create the shared queue instance here
    , frameDataQueue_(std::make_shared<SpscQueue<FrameData>>(FRAME_QUEUE_CAPACITY))
    // Log queue state immediately after creation
    , connectionManager_() // Connection manager needs to be initialized before LeapInput
    // Initialize LeapSorter here with its lambda
    , leapSorter_([this](const std::string& serial, const FrameData& frame) {
                      // This lambda is called by LeapSorter when it finishes processing
                      // It routes the frame to the next stage (DataProcessor)
                      if(dataProcessor_) dataProcessor_->processData(serial, frame);
                  })
{ // Constructor Body starts here
    // Log the state of frameDataQueue_ BEFORE passing it to LeapInput
    if (logger_) {
        logger_->log(frameDataQueue_ ? "AppCore: frameDataQueue_ created successfully." : "AppCore: frameDataQueue_ is NULL after make_shared!");
    } else {
        OutputDebugStringA(frameDataQueue_ ? "AppCore: frameDataQueue_ created successfully (logger null).\n" : "AppCore: frameDataQueue_ is NULL after make_shared! (logger null).\n");
    }

    // Ensure logger is valid before proceeding (already checked below, but good place)
    if (!logger_) {
        OutputDebugStringA("FATAL ERROR: AppCore logger is null before LeapInput creation!\n");
        throw std::runtime_error("AppCore logger is null");
    }
    // Ensure queue is valid before creating LeapInput
    if (!frameDataQueue_) {
        logger_->log("FATAL ERROR: AppCore frameDataQueue_ is null before LeapInput creation!");
        throw std::runtime_error("AppCore frameDataQueue_ is null before LeapInput creation");
    }

    // Initialize LeapInput here in the constructor body instead of initializer list
    // to ensure frameDataQueue_ is definitely initialized and logged first.
    try {
        leapInput_ = std::make_unique<LeapInput>(connectionManager_.getConnection(), frameDataQueue_);
    } catch (const std::exception& e) {
        logger_->log("FATAL ERROR: Failed to construct LeapInput: " + std::string(e.what()));
        throw; // Re-throw exception
    }

    // Assign frameSource_ to point to leapInput_ as IFrameSource
    frameSource_ = static_cast<IFrameSource*>(static_cast<LeapInput*>(leapInput_.get()));
    if (!logger_) {
        OutputDebugStringA("FATAL ERROR: AppCore constructed with null logger!\n");
        throw std::invalid_argument("AppLogger cannot be null in AppCore constructor");
    }
    if (!configManager_) {
        logger_->log("FATAL ERROR: AppCore constructed with null ConfigManager!");
        throw std::invalid_argument("ConfigManagerInterface cannot be null in AppCore constructor");
    }

    logger_->log("Initializing AppCore...");

    try {
        logger_->log("Initializing DataProcessor...");
        dataProcessor_ = std::make_unique<DataProcessor>(
            configManager_->getDeviceAliasManager(),
            [this](const OscMessage& message) {
                if(logger_) {
                    logger_->log("[DP-CALLBACK] Addr: " + message.address + " #Vals: " + std::to_string(message.values.size()));
                } else {
                    std::cout << "[DP-CALLBACK] Addr: " << message.address << " #Vals: " << message.values.size() << std::endl;
                }
                
                if(logger_) logger_->log("AppCore: Routing OSC message via ITransportSink.");
                if (oscSender_) {
                    oscSender_->sendOscMessage(message);
                }
            },
            [this](const FrameData& frame) { 
                uiManager_.handleTrackingData(frame); 
            },
            logger_
        );
         logger_->log("DataProcessor initialized successfully.");
    } catch (const std::exception& e) {
         logger_->log("FATAL ERROR: Failed to initialize DataProcessor: " + std::string(e.what()));
         throw std::runtime_error("DataProcessor initialization failed: " + std::string(e.what()));
    }

    auto aliasLookup = [this](const std::string& serial) -> std::string {
        return configManager_->getDeviceAliasManager().getOrAssignAlias(serial);
    };
    uiManager_.setAliasLookupFunction(aliasLookup);
    
    if (!configManager_->loadConfig()) {
        logger_->log("WARN: Failed to load config file. Using defaults...");
    }
    logger_->log("Configuration loaded.");

    oscSender_ = std::make_unique<OscSender>(configManager_->getOscIp(), configManager_->getOscPort());
    logger_->log("OSC Sender created: IP=" + configManager_->getOscIp() + ", Port=" + std::to_string(configManager_->getOscPort()));

    // +++ Initialize OscController (Simplified) +++
    // Cast configManager_ dependency
    auto* configInterface = dynamic_cast<ConfigManagerInterface*>(configManager_.get());
    if (!configInterface) {
        logger_->log("FATAL ERROR: Could not cast configManager_ to ConfigManagerInterface* during OscController init!");
        throw std::runtime_error("Failed to cast configManager_ to ConfigManagerInterface*");
    }

    try {
        // Pass the ITransportSink directly (no cast needed for sender)
        // Pass the AppLogger instance (logger_) as the third argument
        oscController_ = std::make_unique<OscController>(*oscSender_, *configInterface, logger_);
        logger_->log("OscController initialized successfully.");
    } catch (const std::exception& e) {
        logger_->log("FATAL ERROR: Failed to initialize OscController: " + std::string(e.what()));
        throw; // Re-throw
    }
    // +++ End OscController Initialization +++

    uiController_ = std::make_unique<UIController>(
        leapSorter_,
        *configManager_, // Pass the dereferenced shared_ptr (IConfigStore)
        logger_
    );
    logger_->log("UIController created.");

    if (uiController_) {
        uiController_->setHandAssignmentCallback(
            [this](const std::string& serial, const std::string& hand) {
                logger_->log("AppCore: Handling assignment request for SN: " + serial + " to Hand: " + hand);
                // Create and dispatch the UI update event
                DeviceHandAssignedEvent event{serial, DeviceHandAssignedEvent::stringToHandType(hand)};
                uiManager_.handleDeviceHandAssigned(event);
                logger_->log("AppCore: Hand assignment UI event dispatched.");
            }
        );
        logger_->log("UIController hand assignment callback set.");

        uiController_->setConfigUpdateCallback(
            [this](bool p, bool w, bool t, bool i, bool m, bool r, bool pi, 
                   bool po, bool pv, bool pn, bool vt, bool fie,
                   bool pinch, bool grab)
            {
                logger_->log("AppCore: Received filter update from UIController.");
                if (dataProcessor_) {
                    dataProcessor_->setFilterSettings(p, w, t, i, m, r, pi, po, pv, pn, vt, fie, pinch, grab);
                    logger_->log("AppCore: Updated DataProcessor filter settings.");
                } else {
                     logger_->log("ERROR: AppCore: Cannot update filters, DataProcessor is null!");
                }
            }
        );
        logger_->log("UIController config update callback set.");

        uiController_->setOscSettingsUpdateCallback(
            [this](const std::string& newIp, int newPort) {
                logger_->log("AppCore: Received OSC settings update: IP=" + newIp + ", Port=" + std::to_string(newPort));
                configManager_->setOscIp(newIp);
                configManager_->setOscPort(newPort);
                if (oscSender_) {
                    oscSender_->updateTarget(newIp, newPort);
                }
                logger_->log("AppCore: Updated ConfigManager and live ITransportSink target.");
            }
        );
        logger_->log("UIController OSC Settings update callback set.");
        
        uiController_->initializeOscSettings(configManager_->getOscIp(), configManager_->getOscPort());
        logger_->log("UIController OSC state initialized.");
        uiController_->initializeAllFilters();
        logger_->log("UIController filters initialized (and initial DataProcessor update triggered).");

        uiManager_.setUIController(uiController_.get());
        logger_->log("UIController instance set in MainAppWindow.");
        
    } else {
        logger_->log("ERROR: AppCore: UIController is null! Cannot set callbacks or UI state.");
    }

    // Connect device status callbacks
    leapInput_->setDeviceConnectedCallback([this](const LeapPoller::DeviceInfo& info) {
        this->handleDeviceConnected(info);
    });
    leapInput_->setDeviceLostCallback([this](const std::string& serialNumber) {
        this->handleDeviceLost(serialNumber);
    });
    logger_->log("Leap event callbacks connected.");

    logger_->log("AppCore initialization complete.");
}

AppCore::~AppCore() {
    if (logger_) logger_->log("AppCore shutting down...");
    if (isRunning_.load()) {
        stop();
    }
    if (logger_) logger_->log("AppCore shutdown complete.");
}

void AppCore::start() {
    if (!logger_) { OutputDebugStringA("AppCore::start() called with null logger!\n"); return; }
    if (isRunning_.exchange(true)) {
        logger_->log("AppCore::start() called, but already running.");
        return;
    }
    logger_->log("AppCore starting LeapInput...");
    try {
        leapInput_->start();
    // frameSource_ is ready for getNextFrame
        logger_->log("LeapInput started.");
    } catch (const std::exception& e) {
        logger_->log("ERROR starting LeapInput: " + std::string(e.what()));
        isRunning_ = false;
        throw;
    }
}

void AppCore::stop() {
    if (!logger_) { OutputDebugStringA("AppCore::stop() called with null logger!\n"); return; }
    if (!isRunning_.exchange(false)) {
         logger_->log("AppCore::stop() called, but not running.");
        return;
    }
    logger_->log("AppCore stopping..."); // Log 1

    logger_->log("Requesting LeapInput stop..."); // Log 2
    leapInput_->stop();
    logger_->log("LeapInput stop completed."); // Log 3

    // frameSource_ no longer valid after stop
    logger_->log("LeapInput thread joined."); // Log 4 (Renamed for clarity)

    if (configManager_) {
        logger_->log("Saving configuration..."); // Log 5
        if (configManager_->saveConfig()) {
            logger_->log("Configuration saved successfully."); // Log 6a
        } else {
            logger_->log("ERROR: Failed to save configuration."); // Log 6b
        }
    } else {
        logger_->log("WARNING: Cannot save config, ConfigManager is null."); // Log 6c
    }
    logger_->log("AppCore stopped."); // Log 7
}

void AppCore::handleDeviceConnected(const LeapPoller::DeviceInfo& info) {
    if (!logger_) return;
    logger_->log("AppCore: Device connected: " + info.serialNumber);
    uiManager_.handleDeviceConnected({info.serialNumber});

    if (!configManager_) { logger_->log("ERROR: ConfigManager is null in handleDeviceConnected"); return; }
    auto& aliasManager = configManager_->getDeviceAliasManager();
    std::string alias = aliasManager.getOrAssignAlias(info.serialNumber);
    logger_->log("AppCore: Device " + info.serialNumber + " assigned alias: " + alias);

    std::string defaultHand = configManager_->getDefaultHandAssignment(info.serialNumber);
    if (!defaultHand.empty() && defaultHand != "none") {
        logger_->log("AppCore: Applying default hand assignment '" + defaultHand + "' from config to device " + info.serialNumber);
        leapSorter_.setDeviceHand(info.serialNumber, defaultHand);
        uiManager_.handleDeviceHandAssigned(DeviceHandAssignedEvent(info.serialNumber, DeviceHandAssignedEvent::stringToHandType(defaultHand)));
    }
}

void AppCore::handleDeviceLost(const std::string& serialNumber) {
    if (!logger_) return;
    logger_->log("AppCore: Device lost: " + serialNumber);
    uiManager_.handleDeviceLost({serialNumber});
}

void AppCore::emitTestFrame(const std::string& deviceId, const FrameData& frame) {
    if (!logger_) return;
    logger_->log("AppCore: Emitting test frame for device: " + deviceId);
    leapSorter_.processFrame(deviceId, frame);
}

int AppCore::processPendingFrames() {
    if (!isRunning_ || !frameDataQueue_) return 0; // Safety checks, return 0 if not running or queue null

    std::optional<FrameData> frameOpt;
    // Drain the queue (process up to N frames per tick? or all?)
    // Let's process all available for now.
    int processedCount = 0;
    while ((frameOpt = frameDataQueue_->try_pop())) {
         FrameData& frame = *frameOpt; // Get the frame data

         // Feed the frame into the pipeline (LeapSorter is a direct member, guaranteed to exist)
         leapSorter_.processFrame(frame.deviceId, frame); // Pass to sorter
         processedCount++;
    }
    // Optional: Log if many frames were processed (might indicate main thread lag)
    // if (processedCount > 10 && logger_) {
    //     logger_->log("Processed " + std::to_string(processedCount) + " frames in one tick.");
    // }

    return processedCount;
}

OscController* AppCore::getOscController() {
    // Assuming the member is named oscController_ and is a std::unique_ptr
    // Adjust if the member name or type is different (e.g., if it holds OscSenderStage directly)
    return oscController_.get(); 
}
