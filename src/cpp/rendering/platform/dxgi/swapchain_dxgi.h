#pragma once

#include "rendering/platform/swapchain_base.h"
#include <wrl.h>
using Microsoft::WRL::ComPtr;

class Swapchain_DXGI final : public Swapchain_Base
{
public:
    Swapchain_DXGI(uint2 _dims);
    ~Swapchain_DXGI();

    void resize(uint2 _dims) override;
    void flip() override;
    void present() override;

    static const vk::Format C_BackBufferFormat = vk::Format::eR8G8B8A8Unorm;

private:
    ComPtr<struct ID3D12Device2> m_d3dDevice;
    ComPtr<struct IDXGISwapChain3> m_d3dSwapchain;
    std::vector<vk::Image> m_images;
    UniqueHandle<vk::DeviceMemory> m_swapchainMemory[C_SwapchainImageCount];

    void recreateImages(uint2 _dims);
};