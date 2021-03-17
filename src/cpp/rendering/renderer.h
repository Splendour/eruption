#pragma once

#define VULKAN_HPP_ASSERT ASSERT_TRUE
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

class Renderer 
{
public:
    Renderer(const class Window& _window);
    ~Renderer();

    void RenderFrame();

private:
    void chooseAndInitGpu();
    void createResolutionDependentResources();
    void nameImage(vk::Image _img, LiteralString _name);
    void resize(uint2 _newDims);
    void waitForGpuIdle();
    void completeFrame();
    void recordCommands();
    
    static constexpr vk::Format s_backbufferFormat = vk::Format::eB8G8R8A8Srgb;
    static constexpr u64 s_nsGpuTimeout = 1000000000;
    static constexpr u32 s_swapChainImageCount = 3;

    template <typename T>
    using UniqueHandle = vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;

    UniqueHandle<vk::Instance> m_instance;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;
    UniqueHandle<vk::SurfaceKHR> m_surface;
    UniqueHandle<vk::Device> m_device;
    UniqueHandle<vk::SwapchainKHR> m_swapChain;

    UniqueHandle<vk::ImageView> m_swapChainImageViews[s_swapChainImageCount];
    UniqueHandle<vk::Framebuffer> m_swapChainFrameBuffers[s_swapChainImageCount];

    struct VirtualFrame 
    {
        UniqueHandle<vk::CommandPool> m_cmdBufferPool;
        UniqueHandle<vk::CommandBuffer> m_defaultCmdBuffer;
        UniqueHandle<vk::Fence> m_renderFence;
        UniqueHandle<vk::Semaphore> m_presentSemaphore;
        UniqueHandle<vk::Semaphore> m_renderSemaphore;
    };
    VirtualFrame m_virtualFrames[FRAME_LATENCY];
    u64 m_frameNum = 0;
    u32 m_currentFrameIndex = 0;
    u32 m_backBufferIndex = 0;
    VirtualFrame& getCurrentVirtualFrame() { return m_virtualFrames[m_currentFrameIndex]; }
    vk::CommandBuffer& getDefaultCmdBuffer() { return getCurrentVirtualFrame().m_defaultCmdBuffer.get(); };

    UniqueHandle<vk::RenderPass> m_finalPass;

    vk::DispatchLoaderDynamic m_loader;
    vk::PhysicalDevice m_gpu;
    vk::Queue m_gQueue;


    uint2 m_viewportDims;

};
