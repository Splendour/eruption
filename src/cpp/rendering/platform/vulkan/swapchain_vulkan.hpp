#include "common.h"

#include "swapchain_vulkan.h"
#include "globals.h"
#include "rendering/driver.h"

Swapchain_Vulkan::Swapchain_Vulkan(uint2 _dims)
{
    recreateSwapchain(_dims);
}

void Swapchain_Vulkan::resize(uint2 _dims)
{
    recreateSwapchain(_dims);
}

void Swapchain_Vulkan::flip()
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    m_currentIndex = driver.m_device.acquireNextImageKHR(m_swapchain.get(), C_nsGpuTimeout, getNextPresentSemaphore()).value;
}

void Swapchain_Vulkan::present()
{
    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();

    vk::PresentInfoKHR presentinfo;
    presentinfo.setSwapchains({ 1, &m_swapchain.get() });
    presentinfo.setWaitSemaphores({ 1, getRenderSemaphore() });
    presentinfo.setImageIndices({ 1, &m_currentIndex });
    VK_CHECK(driver.m_gQueue.presentKHR(presentinfo));
}

void Swapchain_Vulkan::recreateSwapchain(uint2 _dims)
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
    swapchaininfo.setPresentMode(vk::PresentModeKHR::eFifoRelaxed);
    swapchaininfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    swapchaininfo.setQueueFamilyIndices({ 0, nullptr });
    swapchaininfo.setOldSwapchain(m_swapchain.get());

    m_swapchain = device.createSwapchainKHRUnique(swapchaininfo).value;

    auto swapchainImages = device.getSwapchainImagesKHR(m_swapchain.get()).value;
    initFrameBuffers(swapchainImages, _dims);

    m_currentIndex = C_SwapchainImageCount - 1;
}
