#pragma once

#include "platform/vk_common.h"
#include "swapchain.h"
#include "GLFW/glfw3.h"

class Renderer 
{
public:
    Renderer(const class Window& _window);
    ~Renderer();

    void RenderFrame();

    void await(vk::Semaphore _sem);
    void signal(vk::Semaphore _sem);

private:
    unique_ptr<Swapchain> m_swapChain;

    struct VirtualFrame 
    {
        UniqueHandle<vk::CommandPool> m_cmdBufferPool;
        UniqueHandle<vk::CommandBuffer> m_defaultCmdBuffer;
        UniqueHandle<vk::Fence> m_renderFence;
    };
    VirtualFrame m_virtualFrames[FRAME_LATENCY];

    u64 m_frameNum = 0;
    u32 m_currentFrameIndex = 0;
    uint2 m_viewportDims;

    void createResolutionDependentResources();
    void resize(uint2 _newDims);
    void waitForGpuIdle();
    void completeFrame();
    void recordCommands();
    VirtualFrame& getCurrentVirtualFrame() { return m_virtualFrames[m_currentFrameIndex]; }
    vk::CommandBuffer& getDefaultCmdBuffer() { return getCurrentVirtualFrame().m_defaultCmdBuffer.get(); };
};
