#pragma once

#define VULKAN_HPP_ASSERT ASSERT_TRUE
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

template <typename T>
using UniqueHandle = vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;

#define VK_CHECK(x) VERIFY_TRUE(vk::Result::eSuccess == x)

static constexpr u64 C_nsGpuTimeout = 1000000000;
static constexpr vk::Format C_BackBufferFormat = vk::Format::eB8G8R8A8Srgb;
