#pragma once

#include "rendering/platform/swapchain_base.h"

class VulkanSwapchain final : public Swapchain_Base 
{
public:
    VulkanSwapchain(uint2 _dims);

    void resize(uint2 _dims) override;
    void flip(vk::Semaphore _sem) override;
    void present(vk::Semaphore _sem) override;

private:
    void recreateSwapchain(uint2 _dims);

    UniqueHandle<vk::SwapchainKHR> m_swapchain;
};