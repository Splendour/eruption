#pragma once

#include "rendering/platform/swapchain_base.h"

class Swapchain_DXGI final : public Swapchain_Base
{
public:
    void resize(uint2 _dims) override;
    void flip(vk::Semaphore _sem) override;
    void present(vk::Semaphore _sem) override;
    vk::Framebuffer getCurrentFrameBuffer() override;
};