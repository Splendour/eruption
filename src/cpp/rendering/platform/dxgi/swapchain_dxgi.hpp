#include "common.h"

#include "swapchain_dxgi.h"

void Swapchain_DXGI::resize(uint2 _dims)
{
    UNREFERENCED_PARAMETER(_dims);
}

void Swapchain_DXGI::flip(vk::Semaphore _sem)
{
    UNREFERENCED_PARAMETER(_sem);
}

void Swapchain_DXGI::present(vk::Semaphore _sem)
{
    UNREFERENCED_PARAMETER(_sem);
}

vk::Framebuffer Swapchain_DXGI::getCurrentFrameBuffer()
{
    return vk::Framebuffer();
}
