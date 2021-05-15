#include "common.h"

#include "renderer.h"
#include "window/window.h"
#include "driver.h"
#include "globals.h"
#include "vma/vk_mem_alloc.h"
#include "shader.h"

Renderer::Renderer(const Window& _window)
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();

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

    for (u32 i = 0; i < FRAME_LATENCY; ++i) {
        vk::FenceCreateInfo fenceinfo;
        m_virtualFrames[i].m_renderFence = driver.m_device.createFenceUnique(fenceinfo).value;
    }

    {
        m_viewportDims = _window.getDims();
        createResolutionDependentResources();
    }

    {
        initPSO();
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

    m_swapChain->flip();
    driver.m_device.resetCommandPool(getCurrentVirtualFrame().m_cmdBufferPool.get());
    getDefaultCmdBuffer().reset();

    recordCommands();

    vk::SubmitInfo submitinfo;
    submitinfo.setCommandBuffers({ 1, &getDefaultCmdBuffer() });
    submitinfo.setSignalSemaphores({ 1, m_swapChain->getRenderSemaphore() });
    submitinfo.setWaitSemaphores({ 1, m_swapChain->getPresentSemaphore() });
    vk::PipelineStageFlags dstmask = vk::PipelineStageFlagBits::eBottomOfPipe;
    submitinfo.setPWaitDstStageMask(&dstmask);
    VK_CHECK(driver.m_gQueue.submit(submitinfo, getCurrentVirtualFrame().m_renderFence.get()));

    completeFrame();
}

void Renderer::await(vk::Semaphore _sem)
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    vk::SubmitInfo submitinfo;
    submitinfo.setCommandBuffers({ 0, nullptr });
    submitinfo.setSignalSemaphores({ 0, VK_NULL_HANDLE });
    submitinfo.setWaitSemaphores({ 1, &_sem });
    vk::PipelineStageFlags dstmask = vk::PipelineStageFlagBits::eTopOfPipe;
    submitinfo.setPWaitDstStageMask(&dstmask);
    VK_CHECK(driver.m_gQueue.submit(submitinfo));   
}

void Renderer::signal(vk::Semaphore _sem)
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    vk::SubmitInfo submitinfo;
    submitinfo.setCommandBuffers({ 0, nullptr });
    submitinfo.setSignalSemaphores({ 1, &_sem });
    submitinfo.setWaitSemaphores({ 0, VK_NULL_HANDLE });
    vk::PipelineStageFlags dstmask = vk::PipelineStageFlagBits::eTopOfPipe;
    submitinfo.setPWaitDstStageMask(&dstmask);
    VK_CHECK(driver.m_gQueue.submit(submitinfo));
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

    m_swapChain->present();

    m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAME_LATENCY;
    m_frameNum++;

    // The resources for the first FRAME_LATENCY frames are ready, but there is nothing to signal the fence
    // TODO: find a more elegant way to handle it
    if (m_frameNum >= FRAME_LATENCY) {
        VK_CHECK(driver.m_device.waitForFences({ 1, &getCurrentVirtualFrame().m_renderFence.get() }, true, C_nsGpuTimeout));
        driver.m_device.resetFences({ 1, &getCurrentVirtualFrame().m_renderFence.get() });
    }
}

void Renderer::recordCommands()
{
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    VK_CHECK(getDefaultCmdBuffer().begin(cmdBeginInfo));

    vk::ClearValue clearValue;
    float flash = abs(sin(m_frameNum / 144.f));
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

    getDefaultCmdBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, m_pso.get());
    vk::Viewport vp;
    vp.width  = static_cast<float>(m_viewportDims.x);
    vp.height = static_cast<float>(m_viewportDims.y);
    vp.maxDepth = 1.0f;
    vk::Rect2D scissor;
    scissor.extent.setWidth(m_viewportDims.x);
    scissor.extent.setHeight(m_viewportDims.y);
    getDefaultCmdBuffer().setViewport(0, vp);
    getDefaultCmdBuffer().setScissor(0, scissor);
    getDefaultCmdBuffer().draw(3, 1, 0, 0);

    getDefaultCmdBuffer().endRenderPass();
    VK_CHECK(getDefaultCmdBuffer().end());
}

void Renderer::initPSO()
{
    ShaderManager shaderManager = globals::getRef<ShaderManager>();

    {
        vk::PipelineShaderStageCreateInfo stageinfo[2];
        ShaderID vsID("depth_pass.hlsl", ShaderType::vs, "vs_main");
        {
            vk::ShaderModule s = shaderManager.getShaderModule(vsID);
            vk::PipelineShaderStageCreateInfo vsinfo;
            vsinfo.setModule(s);
            vsinfo.setPName(vsID.m_entryPoint);
            vsinfo.setStage(vk::ShaderStageFlagBits::eVertex);
            stageinfo[0] = vsinfo;
        }

        ShaderID psID("depth_pass.hlsl", ShaderType::ps, "ps_main");
        {
            vk::ShaderModule s = shaderManager.getShaderModule(psID);
            vk::PipelineShaderStageCreateInfo psinfo;
            psinfo.setModule(s);
            psinfo.setPName(psID.m_entryPoint);
            psinfo.setStage(vk::ShaderStageFlagBits::eFragment);
            stageinfo[1] = psinfo;
        }
        
        vk::PipelineVertexInputStateCreateInfo vtxinfo;
        vk::PipelineInputAssemblyStateCreateInfo iainfo;
        iainfo.setTopology(vk::PrimitiveTopology::eTriangleList);

        vk::PipelineRasterizationStateCreateInfo rastinfo;
        rastinfo.setPolygonMode(vk::PolygonMode::eFill);
        rastinfo.setCullMode(vk::CullModeFlagBits::eBack);
        rastinfo.setLineWidth(1.0f);

        // https://stackoverflow.com/questions/58753504/vulkan-front-face-winding-order
        // The origin for Vulkan is top-left
        // Invert winding order by setting clockwise as front face
        rastinfo.setFrontFace(vk::FrontFace::eClockwise);

        

        vk::PipelineMultisampleStateCreateInfo msinfo;
        vk::PipelineColorBlendAttachmentState attachmentBlendState;
        attachmentBlendState.setColorWriteMask(vk::ColorComponentFlagBits::eA
            | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB);
        attachmentBlendState.setBlendEnable(false);
        attachmentBlendState.setSrcColorBlendFactor(vk::BlendFactor::eOne);
        attachmentBlendState.setDstColorBlendFactor(vk::BlendFactor::eZero);
        attachmentBlendState.setColorBlendOp(vk::BlendOp::eAdd);
        attachmentBlendState.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
        attachmentBlendState.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
        attachmentBlendState.setAlphaBlendOp(vk::BlendOp::eAdd);

        vk::PipelineColorBlendStateCreateInfo blendState;
        blendState.setLogicOpEnable(false);
        blendState.setLogicOp(vk::LogicOp::eCopy);
        blendState.setAttachments({ 1, &attachmentBlendState });

        vk::PipelineDepthStencilStateCreateInfo depthState;
        depthState.setDepthTestEnable(false);
        depthState.setDepthWriteEnable(false);
        depthState.setDepthCompareOp(vk::CompareOp::eLess);
        depthState.setStencilTestEnable(false);
        depthState.setMinDepthBounds(0.f);
        depthState.setMaxDepthBounds(1.f);
        depthState.setDepthBoundsTestEnable(true);

        vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        SmallVector<vk::DynamicState, 4> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        dynamicStateInfo.setDynamicStates({ dynamicStates.size(), dynamicStates.data() });

        vk::PipelineLayoutCreateInfo layoutinfo;
        DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
        m_pipelineLayout = driver.m_device.createPipelineLayoutUnique(layoutinfo).value;

        vk::PipelineViewportStateCreateInfo viewportState;
        vk::Viewport viewport;
        vk::Rect2D scissor;
        viewportState.setViewports({ 1, &viewport });
        viewportState.setScissors({ 1, &scissor });
        

        vk::GraphicsPipelineCreateInfo pipelineinfo;
        pipelineinfo.setStages({ 2, stageinfo });
        pipelineinfo.setPVertexInputState(&vtxinfo);
        pipelineinfo.setPInputAssemblyState(&iainfo);
        pipelineinfo.setPRasterizationState(&rastinfo);
        pipelineinfo.setPMultisampleState(&msinfo);
        pipelineinfo.setPColorBlendState(&blendState);
        pipelineinfo.setPDepthStencilState(&depthState);
        pipelineinfo.setPDynamicState(&dynamicStateInfo);
        pipelineinfo.setPViewportState(&viewportState);
        
        pipelineinfo.setLayout(m_pipelineLayout.get());

        // Framebuffers and graphics pipelines are created based on a specific render pass object.
        // They must only be used with that render pass object, or one compatible with it.
        pipelineinfo.setRenderPass(m_swapChain->getCompositionRenderPass());

        m_pso = driver.m_device.createGraphicsPipelineUnique(nullptr, pipelineinfo).value;
    }
}
