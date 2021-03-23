#include "common.h"

#include "renderer.h"
#include "window/window.h"
#include "driver.h"
#include "globals.h"

Renderer::Renderer(const Window& _window)
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();

    {
        for (u32 i = 0; i < FRAME_LATENCY; ++i) {
            vk::CommandPoolCreateInfo poolinfo;
            poolinfo.setQueueFamilyIndex(driver.m_queueIndex);
            poolinfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            m_virtualFrames[i].m_cmdBufferPool = driver.m_device.createCommandPoolUnique(poolinfo).value;

            vk::CommandBufferAllocateInfo allocateinfo;
            allocateinfo.setCommandBufferCount(1);
            allocateinfo.setCommandPool(m_virtualFrames[i].m_cmdBufferPool.get());
            allocateinfo.setLevel(vk::CommandBufferLevel::ePrimary);
            auto cmdBuffers = driver.m_device.allocateCommandBuffersUnique(allocateinfo).value;
            m_virtualFrames[i].m_defaultCmdBuffer = std::move(cmdBuffers[0]);
        }
    }

    {
        for (u32 i = 0; i < FRAME_LATENCY; ++i) {
            vk::SemaphoreCreateInfo semaphoreinfo;

            m_virtualFrames[i].m_presentSemaphore = driver.m_device.createSemaphoreUnique(semaphoreinfo).value;
            m_virtualFrames[i].m_renderSemaphore = driver.m_device.createSemaphoreUnique(semaphoreinfo).value;

            vk::FenceCreateInfo fenceinfo;
            m_virtualFrames[i].m_renderFence = driver.m_device.createFenceUnique(fenceinfo).value;
        }
    }

    {
        m_viewportDims = _window.getDims();
        createResolutionDependentResources();
    }

}

Renderer::~Renderer()
{
    waitForGpuIdle();
}

void Renderer::RenderFrame()
{
    Window* window = globals::getPtr<Window>();
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    if (window->isMinimized())
        return;

    uint2 windowDims = window->getDims();

    uint2 newDims = windowDims;
    if (newDims.x != m_viewportDims.x || newDims.y != m_viewportDims.y) {
        resize(newDims);
    }

    m_swapChain->flip(getCurrentVirtualFrame().m_presentSemaphore.get());

    driver.m_device.resetCommandPool(getCurrentVirtualFrame().m_cmdBufferPool.get());
    getDefaultCmdBuffer().reset();

    recordCommands();

    vk::SubmitInfo submitinfo;
    submitinfo.setCommandBuffers({ 1, &getDefaultCmdBuffer() });
    submitinfo.setSignalSemaphores({ 1, &getCurrentVirtualFrame().m_renderSemaphore.get() });
    submitinfo.setWaitSemaphores({ 1, &getCurrentVirtualFrame().m_presentSemaphore.get() });
    vk::PipelineStageFlags dstmask = vk::PipelineStageFlagBits::eBottomOfPipe;
    submitinfo.setPWaitDstStageMask(&dstmask);
    VK_CHECK(driver.m_gQueue.submit(submitinfo, getCurrentVirtualFrame().m_renderFence.get()));

    completeFrame();
}


void Renderer::createResolutionDependentResources()
{
    if (!m_swapChain) {
        m_swapChain = std::make_unique<Swapchain>(m_viewportDims);
    } else {
        m_swapChain->resize(m_viewportDims);
    };
}

void Renderer::resize(uint2 _newDims)
{
    m_viewportDims = _newDims;
    logInfo("Renderer resize to {}x{}", m_viewportDims.x, m_viewportDims.y);
    waitForGpuIdle();
    createResolutionDependentResources();
}

void Renderer::waitForGpuIdle()
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    VK_CHECK(driver.m_device.waitIdle());
}

void Renderer::completeFrame()
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    m_swapChain->present(getCurrentVirtualFrame().m_renderSemaphore.get());

    VK_CHECK(driver.m_device.waitForFences({ 1, &getCurrentVirtualFrame().m_renderFence.get() }, true, C_nsGpuTimeout));
    driver.m_device.resetFences({ 1, &getCurrentVirtualFrame().m_renderFence.get() });
    m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAME_LATENCY;
    m_frameNum++;
}

void Renderer::recordCommands()
{
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    VK_CHECK(getDefaultCmdBuffer().begin(cmdBeginInfo));

    vk::ClearValue clearValue;
    float flash = 0.7f + 3 * abs(sin(m_frameNum / 120.f));
    clearValue.color.setFloat32({ { 0.1f, flash, 0.2f, 1.0f } });
    vk::Rect2D area;
    area.setOffset({ 0, 0 });
    area.setExtent({ m_viewportDims.x, m_viewportDims.y });

    vk::RenderPassBeginInfo rpinfo;
    rpinfo.setClearValues({ 1, &clearValue });
    rpinfo.setRenderPass(m_swapChain->getCompositionRenderPass());
    rpinfo.setRenderArea(area);
    rpinfo.setFramebuffer(m_swapChain->getCurrentFrameBuffer());

    getDefaultCmdBuffer().beginRenderPass(rpinfo, vk::SubpassContents::eInline);
    getDefaultCmdBuffer().endRenderPass();
    VK_CHECK(getDefaultCmdBuffer().end());
}
