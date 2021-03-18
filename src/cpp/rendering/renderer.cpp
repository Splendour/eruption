#include "common.h"

#include "renderer.h"
#include "window/window.h"
#include "GLFW/glfw3.h"
#include "globals.h"

#ifndef _NDEBUG
#define ENABLE_VALIDATION
#endif

#ifdef ENABLE_VALIDATION
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /* _messageType */,
    const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData,
    void* /* _pUserData */)
{
    if (_messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        logError("[Vulkan validation error] {}", _pCallbackData->pMessage);
    else
        logInfo("[Vulkan validation] {}", _pCallbackData->pMessage);

    return VK_FALSE;
}
#endif

#define VK_CHECK(x) VERIFY_TRUE(vk::Result::eSuccess == x)

Renderer::Renderer(const Window& _window)
{
    {
        vk::ApplicationInfo appinfo;
        appinfo.setPApplicationName(APP_NAME);
        appinfo.setApiVersion(VK_MAKE_VERSION(0, 0, 1));
        appinfo.setPEngineName("");
        appinfo.setApiVersion(VK_API_VERSION_1_2);

        vk::InstanceCreateInfo instanceinfo;
        instanceinfo.setPApplicationInfo(&appinfo);

        SmallVector<const char*, 8> instanceExtensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
            //VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            //VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME,
        };

        u32 glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (u32 i = 0; i < glfwExtensionCount; ++i)
            instanceExtensions.push_back(glfwExtensions[i]);

        instanceinfo.setPEnabledExtensionNames({ instanceExtensions.size(), instanceExtensions.data() });

        SmallVector<const char*, 8> instanceLayers = {
#ifdef ENABLE_VALIDATION
            "VK_LAYER_KHRONOS_validation",
#endif
            "VK_LAYER_KHRONOS_synchronization2",
        };


        instanceinfo.setPEnabledLayerNames({ instanceLayers.size(), instanceLayers.data() });

        m_instance = vk::createInstanceUnique(instanceinfo).value;
    }

    {
        vk::DynamicLoader dl;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        m_loader.init(m_instance.get(), vkGetInstanceProcAddr);
    }

#ifdef ENABLE_VALIDATION
    {
        vk::DebugUtilsMessengerCreateInfoEXT debuginfo;
        debuginfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            //| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
        debuginfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debuginfo.setPfnUserCallback(vulkanDebugCallback);

        m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(debuginfo, nullptr, m_loader).value;
    }
#endif

    {
        VkSurfaceKHR surf;
        VERIFY_TRUE(VK_SUCCESS == glfwCreateWindowSurface(m_instance.get(), _window.getGlfwHandle(), nullptr, &surf));
        m_surface = UniqueHandle<vk::SurfaceKHR>(surf, m_instance.get());
    }

    {
        chooseAndInitGpu();
        vk::PhysicalDeviceProperties props = m_gpu.getProperties();
        logInfo("GpuName: {}", props.deviceName);
        logInfo("GpuDriver: {}", props.driverVersion);
    }

    {
        SmallVector<const char*, 8> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
        };
        vk::DeviceCreateInfo deviceinfo;
        deviceinfo.setPEnabledExtensionNames({ deviceExtensions.size(), deviceExtensions.data() });

        vk::PhysicalDeviceSynchronization2FeaturesKHR sync2;
        sync2.setSynchronization2(true);
        deviceinfo.setPNext(&sync2);

        auto queueprops = m_gpu.getQueueFamilyProperties();
        u32 qindex = 0;
        for (; qindex < queueprops.size(); ++qindex) {
            if (queueprops[qindex].queueFlags | vk::QueueFlagBits::eGraphics)
                break;
        }

        vk::DeviceQueueCreateInfo queueinfo;
        queueinfo.setQueueFamilyIndex(qindex);
        queueinfo.setQueueCount(1);
        SmallVector<float, 4> priorities = {
            1.0f,
        };
        queueinfo.setQueuePriorities({ priorities.size(), priorities.data() });
        deviceinfo.setQueueCreateInfos(queueinfo);

        m_device = m_gpu.createDeviceUnique(deviceinfo).value;
        m_loader.init(m_device.get());

        vk::Queue q = m_device->getQueue(qindex, 0);
        m_gQueue = q;

        VERIFY_TRUE(m_gpu.getSurfaceSupportKHR(qindex, m_surface.get()).value);

        for (u32 i = 0; i < FRAME_LATENCY; ++i) {
            vk::CommandPoolCreateInfo poolinfo;
            poolinfo.setQueueFamilyIndex(qindex);
            poolinfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            m_virtualFrames[i].m_cmdBufferPool = m_device->createCommandPoolUnique(poolinfo).value;

            vk::CommandBufferAllocateInfo allocateinfo;
            allocateinfo.setCommandBufferCount(1);
            allocateinfo.setCommandPool(m_virtualFrames[i].m_cmdBufferPool.get());
            allocateinfo.setLevel(vk::CommandBufferLevel::ePrimary);
            auto cmdBuffers = m_device->allocateCommandBuffersUnique(allocateinfo).value;
            m_virtualFrames[i].m_defaultCmdBuffer = std::move(cmdBuffers[0]);
        }
    }

    {
        for (u32 i = 0; i < FRAME_LATENCY; ++i) {
            vk::SemaphoreCreateInfo semaphoreinfo;

            m_virtualFrames[i].m_presentSemaphore = m_device->createSemaphoreUnique(semaphoreinfo).value;
            m_virtualFrames[i].m_renderSemaphore = m_device->createSemaphoreUnique(semaphoreinfo).value;

            vk::FenceCreateInfo fenceinfo;
            m_virtualFrames[i].m_renderFence = m_device->createFenceUnique(fenceinfo).value;
        }
    }

    {
        vk::AttachmentDescription attachment;
        attachment.setFormat(s_backbufferFormat);
        attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference attachmentRef;
        attachmentRef.setAttachment(0);
        attachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpass;
        subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
        subpass.setColorAttachments({ 1, &attachmentRef });

        vk::SubpassDependency dep;
        dep.setDstSubpass(0);
        dep.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR);
        dep.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        dep.setSrcSubpass(VK_SUBPASS_EXTERNAL);
        dep.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dep.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

        vk::RenderPassCreateInfo passinfo;
        passinfo.setAttachments({ 1, &attachment });
        passinfo.setSubpasses({ 1, &subpass });
        passinfo.setDependencies({ 1, &dep });
        m_finalPass = m_device->createRenderPassUnique(passinfo).value;
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
    if (window->isMinimized())
        return;

    uint2 windowDims = window->getDims();

    uint2 newDims = windowDims;
    if (newDims.x != m_viewportDims.x || newDims.y != m_viewportDims.y) {
        resize(newDims);
    }
    m_backBufferIndex = m_device->acquireNextImageKHR(m_swapChain.get(), s_nsGpuTimeout, getCurrentVirtualFrame().m_presentSemaphore.get()).value;

    m_device->resetCommandPool(getCurrentVirtualFrame().m_cmdBufferPool.get());
    getDefaultCmdBuffer().reset();

    recordCommands();

    vk::SubmitInfo submitinfo;
    submitinfo.setCommandBuffers({ 1, &getDefaultCmdBuffer() });
    submitinfo.setSignalSemaphores({ 1, &getCurrentVirtualFrame().m_renderSemaphore.get() });
    submitinfo.setWaitSemaphores({ 1, &getCurrentVirtualFrame().m_presentSemaphore.get() });
    vk::PipelineStageFlags dstmask = vk::PipelineStageFlagBits::eBottomOfPipe;
    submitinfo.setPWaitDstStageMask(&dstmask);
    VK_CHECK(m_gQueue.submit(submitinfo, getCurrentVirtualFrame().m_renderFence.get()));

    completeFrame();
}

void Renderer::chooseAndInitGpu()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance.get(), &deviceCount, nullptr);
    VERIFY_TRUE_MSG(deviceCount, "No gpu detected");
    SmallVector<VkPhysicalDevice, 8> devices(deviceCount);

    vkEnumeratePhysicalDevices(m_instance.get(), &deviceCount, devices.data());
    SmallVector<i32, 8> deviceScores(deviceCount, -1);

    for (u32 i = 0; i < deviceCount; ++i) {
        const VkPhysicalDevice& device = devices[i];
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        SmallVector<VkQueueFamilyProperties, 8> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        if (!deviceFeatures.geometryShader)
            continue;

        bool hasGraphicsQueue = false;
        for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                hasGraphicsQueue = true;
        }

        if (!hasGraphicsQueue)
            continue;

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            deviceScores[i] += 10000;

        deviceScores[i] += deviceProperties.limits.maxImageDimension2D;
    }

    u32 bestDeviceIndex = (u32)std::distance(deviceScores.begin(),
        std::max_element(deviceScores.begin(), deviceScores.end()));

    VERIFY_TRUE_MSG(deviceScores[bestDeviceIndex] >= 0, "No gpu detected");
    m_gpu = devices[bestDeviceIndex];


    #if 0
    u32 extCnt = 0;
    vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &extCnt, nullptr);
    std::vector<VkExtensionProperties> exts(extCnt);
    vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &extCnt, exts.data());

    for (auto ext : exts)
        logInfo(ext.extensionName);
    #endif
}

void Renderer::createResolutionDependentResources()
{
    {
        vk::SwapchainCreateInfoKHR swapchaininfo;
        vk::SurfaceCapabilitiesKHR surfCaps = m_gpu.getSurfaceCapabilitiesKHR(m_surface.get()).value;
        swapchaininfo.setImageFormat(s_backbufferFormat);
        swapchaininfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchaininfo.setImageArrayLayers(1);
        swapchaininfo.setImageExtent({ m_viewportDims.x, m_viewportDims.y });
        swapchaininfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchaininfo.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchaininfo.setSurface(m_surface.get());
        swapchaininfo.setMinImageCount(s_swapChainImageCount);
        swapchaininfo.setPreTransform(surfCaps.currentTransform);
        swapchaininfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapchaininfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchaininfo.setQueueFamilyIndices({ 0, nullptr });
        swapchaininfo.setOldSwapchain(m_swapChain.get());

        m_swapChain = m_device->createSwapchainKHRUnique(swapchaininfo).value;

        auto swapChainImages = m_device->getSwapchainImagesKHR(m_swapChain.get()).value;
        for (u32 i = 0; i < swapChainImages.size(); ++i) {
            nameImage(swapChainImages[i], "Backbuffer image");

            vk::ImageViewCreateInfo viewinfo;
            viewinfo.setViewType(vk::ImageViewType::e2D);
            viewinfo.setImage(swapChainImages[i]);
            viewinfo.setFormat(s_backbufferFormat);
            vk::ImageSubresourceRange sr;
            sr.setAspectMask(vk::ImageAspectFlagBits::eColor);
            sr.setLevelCount(1);
            sr.setLayerCount(1);
            viewinfo.setSubresourceRange(sr);
            m_swapChainImageViews[i] = m_device->createImageViewUnique(viewinfo).value;

            vk::FramebufferCreateInfo fbinfo;
            fbinfo.setRenderPass(m_finalPass.get());
            fbinfo.setAttachments({ 1, &m_swapChainImageViews[i].get() });
            fbinfo.setWidth(m_viewportDims.x);
            fbinfo.setHeight(m_viewportDims.y);
            fbinfo.setLayers(1);
            m_swapChainFrameBuffers[i] = m_device->createFramebufferUnique(fbinfo).value;
        }
    }
}

void Renderer::nameImage(vk::Image _img, LiteralString _name)
{
    vk::DebugUtilsObjectNameInfoEXT info;
    info.setObjectType(vk::ObjectType::eImage);
    info.setObjectHandle((u64)_img.operator VkImage());
    info.setPObjectName(_name);
    VK_CHECK(m_device->setDebugUtilsObjectNameEXT(info, m_loader));
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
    VK_CHECK(m_device->waitIdle());
}

void Renderer::completeFrame()
{

    vk::PresentInfoKHR presentinfo;
    presentinfo.setSwapchains({ 1, &m_swapChain.get() });
    presentinfo.setWaitSemaphores({ 1, &getCurrentVirtualFrame().m_renderSemaphore.get() });
    presentinfo.setImageIndices({ 1, &m_backBufferIndex });
    VK_CHECK(m_gQueue.presentKHR(presentinfo));

    VK_CHECK(m_device->waitForFences({ 1, &getCurrentVirtualFrame().m_renderFence.get() }, true, s_nsGpuTimeout));
    m_device->resetFences({ 1, &getCurrentVirtualFrame().m_renderFence.get() });
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
    rpinfo.setRenderPass(m_finalPass.get());
    rpinfo.setRenderArea(area);
    rpinfo.setFramebuffer(m_swapChainFrameBuffers[m_backBufferIndex].get());

    getDefaultCmdBuffer().beginRenderPass(rpinfo, vk::SubpassContents::eInline);
    getDefaultCmdBuffer().endRenderPass();
    VK_CHECK(getDefaultCmdBuffer().end());
}
