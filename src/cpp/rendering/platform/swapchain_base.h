#pragma once

#include "vk_common.h"
class Swapchain_Base 
{
public:

    virtual void resize(uint2 _dims) = 0;
    virtual void flip(vk::Semaphore _sem) = 0;
    virtual void present(vk::Semaphore _sem) = 0;

    vk::Framebuffer getCurrentFrameBuffer();

    Swapchain_Base();
    ~Swapchain_Base() {};

    vk::RenderPass getCompositionRenderPass();

protected:
    static constexpr u32 C_SwapchainImageCount = 3;
    UniqueHandle<vk::ImageView> m_swapchainImageViews[C_SwapchainImageCount];
    UniqueHandle<vk::Framebuffer> m_swapchainFrameBuffers[C_SwapchainImageCount];
    UniqueHandle<vk::RenderPass> m_finalPass;
    u32 m_currentIndex = 0;

    void initFrameBuffers(std::vector<vk::Image>& _swapchainImages, uint2 _dims);
};