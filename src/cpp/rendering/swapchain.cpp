#include "common.h"

#include "swapchain.h"

#if SWAPCHAIN_DXGI
#include "platform/dxgi/swapchain_dxgi.hpp"
#else
#include "platform/vulkan/swapchain_vulkan.hpp"
#endif