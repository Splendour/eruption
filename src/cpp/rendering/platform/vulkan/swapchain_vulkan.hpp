#include "common.h"

#include "swapchain_vulkan.h"
#include "globals.h"
#include "rendering/driver.h"

VulkanSwapchain::VulkanSwapchain(uint2 _dims)
{
    recreateSwapchain(_dims);
}

void VulkanSwapchain::resize(uint2 _dims)
{
    recreateSwapchain(_dims);
}

void VulkanSwapchain::flip(vk::Semaphore _sem)
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    m_currentIndex = driver.m_device.acquireNextImageKHR(m_swapchain.get(), C_nsGpuTimeout, _sem).value;
}

void VulkanSwapchain::present(vk::Semaphore _sem)
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();

    vk::PresentInfoKHR presentinfo;
    presentinfo.setSwapchains({ 1, &m_swapchain.get() });
    presentinfo.setWaitSemaphores({ 1, &_sem });
    presentinfo.setImageIndices({ 1, &m_currentIndex });
    VK_CHECK(driver.m_gQueue.presentKHR(presentinfo));
}

vk::Framebuffer VulkanSwapchain::getCurrentFrameBuffer()
{
    return m_swapchainFrameBuffers[m_currentIndex].get();
}

void VulkanSwapchain::recreateSwapchain(uint2 _dims)
{
    Driver& driver = globals::getRef<Driver>();
    auto driverObjects = driver.getDriverObjects();
    auto device = driverObjects.m_device;
    auto surface = driverObjects.m_surface;    

    vk::SwapchainCreateInfoKHR swapchaininfo;
    swapchaininfo.setImageFormat(C_BackBufferFormat);
    swapchaininfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    swapchaininfo.setImageArrayLayers(1);
    swapchaininfo.setImageExtent({ _dims.x, _dims.y });
    swapchaininfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    swapchaininfo.setImageSharingMode(vk::SharingMode::eExclusive);
    swapchaininfo.setSurface(surface);
    swapchaininfo.setMinImageCount(C_SwapchainImageCount);
    swapchaininfo.setPreTransform(driver.getCaps().currentTransform);
    swapchaininfo.setPresentMode(vk::PresentModeKHR::eMailbox);
    swapchaininfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    swapchaininfo.setQueueFamilyIndices({ 0, nullptr });
    swapchaininfo.setOldSwapchain(m_swapchain.get());

    m_swapchain = device.createSwapchainKHRUnique(swapchaininfo).value;

    auto swapchainImages = device.getSwapchainImagesKHR(m_swapchain.get()).value;
    for (u32 i = 0; i < swapchainImages.size(); ++i) {
        driver.nameImage(swapchainImages[i], "Backbuffer image");

        vk::ImageViewCreateInfo viewinfo;
        viewinfo.setViewType(vk::ImageViewType::e2D);
        viewinfo.setImage(swapchainImages[i]);
        viewinfo.setFormat(C_BackBufferFormat);
        vk::ImageSubresourceRange sr;
        sr.setAspectMask(vk::ImageAspectFlagBits::eColor);
        sr.setLevelCount(1);
        sr.setLayerCount(1);
        viewinfo.setSubresourceRange(sr);
        m_swapchainImageViews[i] = device.createImageViewUnique(viewinfo).value;

        vk::FramebufferCreateInfo fbinfo;
        fbinfo.setRenderPass(m_finalPass.get());
        fbinfo.setAttachments({ 1, &m_swapchainImageViews[i].get() });
        fbinfo.setWidth(_dims.x);
        fbinfo.setHeight(_dims.y);
        fbinfo.setLayers(1);
        m_swapchainFrameBuffers[i] = device.createFramebufferUnique(fbinfo).value;
    }
}
