#pragma once

#include "platform/vk_common.h"

#ifdef _WIN32
#define SWAPCHAIN_DXGI 1
#else
#define SWAPCHAIN_DXGI 0
#endif

#if SWAPCHAIN_DXGI
#include "platform/dxgi/swapchain_dxgi.h"
using Swapchain = Swapchain_DXGI;
#else
#include "platform/vulkan/swapchain_vulkan.h"
using Swapchain = Swapchain_Vulkan;
#endif
