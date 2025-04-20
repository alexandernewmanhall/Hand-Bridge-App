#include "OscController.h"
#include "core/interfaces/ITransportSink.hpp"
#include "core/AppLogger.hpp"

OscController::OscController(ITransportSink& sender, ConfigManagerInterface& configMgr, std::shared_ptr<AppLogger> logger)
    : oscSender_(sender), configManager_(configMgr), uiManager_(nullptr), logger_(std::move(logger)) {}

// No inputQueue or ThreadSafeQueue logic remains. All message handling is real-time, latest-only.
