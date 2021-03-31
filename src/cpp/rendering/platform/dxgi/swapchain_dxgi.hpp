#include "common.h"

#include "swapchain_dxgi.h"
#include <dxgi1_6.h>
#include <d3d12.h>
#include "globals.h"
#include "window/window.h"
#include "rendering/driver.h"
#include "rendering/renderer.h"

#define ThrowIfFailed(hr) VERIFY_TRUE(hr >= 0)

Swapchain_DXGI::Swapchain_DXGI(uint2 _dims)
{
    Driver& driver = globals::getRef<Driver>();
    m_images.resize(C_SwapchainImageCount);

    ComPtr<struct IDXGIFactory4> dxgiFactory;
    {
        UINT dxgiFactoryFlags = 0;
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    }

    {
        VkPhysicalDeviceIDPropertiesKHR id = driver.getGpuID();
        ComPtr<struct IDXGIAdapter1> adapter;

        // LUID = locally unique id
        VERIFY_TRUE(id.deviceLUIDValid);
        ThrowIfFailed(dxgiFactory->EnumAdapterByLuid(*(LUID*)id.deviceLUID, IID_PPV_ARGS(&adapter)));

        D3D_FEATURE_LEVEL targetFeatureLevel = D3D_FEATURE_LEVEL_11_1; // min d3d12 feature level
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), targetFeatureLevel, IID_PPV_ARGS(&m_d3dDevice)));
    }

    {
        ComPtr<ID3D12CommandQueue> presentQueue;
        D3D12_COMMAND_QUEUE_DESC presentQueueDesc = {};
        presentQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        presentQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&presentQueueDesc, IID_PPV_ARGS(&presentQueue)));


        constexpr static DXGI_FORMAT C_D3DBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        static_assert(C_BackBufferFormat == vk::Format::eR8G8B8A8Unorm);

        ComPtr<IDXGISwapChain1> swapChain;
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = C_SwapchainImageCount;
        swapChainDesc.Width = _dims.x;
        swapChainDesc.Height = _dims.y;
        swapChainDesc.Format = C_D3DBackBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        HWND nativeHandle = globals::getRef<Window>().getNativeHandle();
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            presentQueue.Get(),
            nativeHandle,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain));

        ThrowIfFailed(swapChain.As(&m_d3dSwapchain));
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation(nativeHandle, DXGI_MWA_NO_ALT_ENTER));
    }

    {
        recreateImages(_dims);
    }
}

Swapchain_DXGI::~Swapchain_DXGI()
{
    Driver& driver = globals::getRef<Driver>();
    vk::Device device = driver.getDriverObjects().m_device;

    for (vk::Image img : m_images) {
        device.destroyImage(img);
    }
}

void Swapchain_DXGI::resize(uint2 _dims)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    m_d3dSwapchain->GetDesc(&desc);
    ThrowIfFailed(m_d3dSwapchain->ResizeBuffers(FRAME_LATENCY, _dims.x, _dims.y, desc.BufferDesc.Format, desc.Flags));
    recreateImages(_dims);
}

void Swapchain_DXGI::flip(vk::Semaphore _sem)
{
    Renderer& renderer = globals::getRef<Renderer>();
    m_currentIndex = m_d3dSwapchain->GetCurrentBackBufferIndex();
    renderer.signal(_sem);
}

void Swapchain_DXGI::present(vk::Semaphore _sem)
{
    Renderer& renderer = globals::getRef<Renderer>();
    renderer.await(_sem);
    bool vsync = true;
    ThrowIfFailed(m_d3dSwapchain->Present(vsync ? 1 : 0, 0));
}

inline void Swapchain_DXGI::recreateImages(uint2 _dims)
{
    Driver& driver = globals::getRef<Driver>();
    vk::Device device = driver.getDriverObjects().m_device;

    for (UniqueHandle<vk::Framebuffer>& fb : m_swapchainFrameBuffers) {
        fb.release();
    }

    for (vk::Image img : m_images) {
        if (img)
            device.destroyImage(img);
    }

    std::array<ComPtr<ID3D12Resource>, C_SwapchainImageCount> d3dFrameBuffers;
    for (u32 i = 0; i < C_SwapchainImageCount; ++i)
        ThrowIfFailed(m_d3dSwapchain->GetBuffer(i, IID_PPV_ARGS(&d3dFrameBuffers[i])));

    for (u32 i = 0; i < d3dFrameBuffers.size(); ++i) {
        vk::ExternalMemoryImageCreateInfo externinfo;
        externinfo.setHandleTypes(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource);

        vk::ImageCreateInfo imageinfo;
        imageinfo.setPNext(&externinfo);
        imageinfo.setImageType(vk::ImageType::e2D);
        imageinfo.setFormat(C_BackBufferFormat);
        //imageinfo.setFormat(vk::Format::eB8G8R8A8Srgb);
        imageinfo.setUsage(vk::ImageUsageFlagBits::eColorAttachment);
        imageinfo.setArrayLayers(1);
        imageinfo.setSamples(vk::SampleCountFlagBits::e1);
        imageinfo.setExtent({ _dims.x, _dims.y, 1 });
        imageinfo.setQueueFamilyIndices({ 0, nullptr });
        imageinfo.setMipLevels(1);

        m_images[i] = driver.getDriverObjects().m_device.createImage(imageinfo).value;
        vk::MemoryRequirements memreq = device.getImageMemoryRequirements(m_images[i]);
        vk::PhysicalDeviceMemoryProperties memprops = driver.getMemProps();

        u32 memindex = 0xffffffff;
        for (u32 m = 0; m < memprops.memoryTypeCount; ++m) {
            bool isSuitable = memreq.memoryTypeBits & (1 << m);
            bool isDeviceMemory = (bool)(memprops.memoryTypes[m].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);
            if (isSuitable && isDeviceMemory) {
                memindex = m;
                break;
            }
        }

        //LPCWSTR handleNames[] = { L"BackBuffer0", L"BackBuffer1", L"BackBuffer2" };
        HANDLE imgHandle = nullptr;
        HRESULT hr = (m_d3dDevice->CreateSharedHandle(d3dFrameBuffers[i].Get(), nullptr, GENERIC_ALL, nullptr, &imgHandle));
        ThrowIfFailed(hr);

        vk::MemoryDedicatedAllocateInfoKHR dedicatedAllocInfo;
        dedicatedAllocInfo.setImage(m_images[i]);

        vk::ImportMemoryWin32HandleInfoKHR win32MemInfo;
        win32MemInfo.setHandle(imgHandle);
        win32MemInfo.setPNext(&dedicatedAllocInfo);
        win32MemInfo.setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource);

        vk::MemoryAllocateInfo meminfo;
        meminfo.setAllocationSize(memreq.size);
        meminfo.setMemoryTypeIndex(memindex);
        meminfo.setPNext(&win32MemInfo);
        
        // FIXME: crash on resize
        m_swapchainMemory[i] = device.allocateMemoryUnique(meminfo).value;
        VK_CHECK(device.bindImageMemory(m_images[i], m_swapchainMemory[i].get(), 0));

    }
    initFrameBuffers(m_images, _dims);
}
