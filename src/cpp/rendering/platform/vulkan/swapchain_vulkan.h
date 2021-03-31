#pragma once

#include "rendering/platform/swapchain_base.h"

class Swapchain_Vulkan final : public Swapchain_Base 
{
public:
    Swapchain_Vulkan(uint2 _dims);

    void resize(uint2 _dims) override;
    void flip(vk::Semaphore _sem) override;
    void present(vk::Semaphore _sem) override;

    static const vk::Format C_BackBufferFormat = vk::Format::eB8G8R8A8Srgb;

private:
    void recreateSwapchain(uint2 _dims);

    UniqueHandle<vk::SwapchainKHR> m_swapchain;
};