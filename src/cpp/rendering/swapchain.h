#pragma once

//#ifdef _WIN32
#if 0
#include "platform/dxgi/swapchain_dxgi.h"
using Swapchain = Swapchain_DXGI;
#else
#include "platform/vulkan/swapchain_vulkan.h"
using Swapchain = VulkanSwapchain;
#endif
