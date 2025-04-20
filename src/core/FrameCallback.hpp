#pragma once
#include <functional>
#include "FrameData.hpp"

using FrameCallback = std::function<void(const FrameData&)>;
