#pragma once

#include "vk_common.h"
class Swapchain_Base 
{
public:

    virtual void resize(uint2 _dims) = 0;
    virtual void flip() = 0;
    virtual void present() = 0;

    vk::Framebuffer getCurrentFrameBuffer();

    Swapchain_Base();
    virtual ~Swapchain_Base() {};

    vk::RenderPass getCompositionRenderPass();

    vk::Semaphore* getPresentSemaphore();
    vk::Semaphore* getRenderSemaphore();

protected:
    static constexpr u32 C_SwapchainImageCount = 3;

    UniqueHandle<vk::ImageView> m_swapchainImageViews[C_SwapchainImageCount];
    UniqueHandle<vk::Framebuffer> m_swapchainFrameBuffers[C_SwapchainImageCount];
    UniqueHandle<vk::RenderPass> m_finalPass;
    u32 m_currentIndex = 0;

    void initFrameBuffers(std::vector<vk::Image>& _swapchainImages, uint2 _dims);

    vk::Semaphore getNextPresentSemaphore();

private:
    // Wait until the image has been presented to render new stuff onto it
    UniqueHandle<vk::Semaphore> m_presentSemaphores[C_SwapchainImageCount];
    // Wait until the image has been renderered to present it
    UniqueHandle<vk::Semaphore> m_renderSemaphores[C_SwapchainImageCount];
};