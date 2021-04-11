#include "common.h"

#include "swapchain_base.h"
#include "rendering/driver.h"
#include "rendering/swapchain.h"
#include "globals.h"

vk::Framebuffer Swapchain_Base::getCurrentFrameBuffer()
{
    return m_swapchainFrameBuffers[m_currentIndex].get();
}

Swapchain_Base::Swapchain_Base()
{
    vk::AttachmentDescription attachment;
    attachment.setFormat(Swapchain::C_BackBufferFormat);
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

    DriverObjects driver = globals::getRef<Driver>().getDriverObjects();
    m_finalPass = driver.m_device.createRenderPassUnique(passinfo).value;

    vk::SemaphoreCreateInfo semaphoreinfo;
    for (u32 i = 0; i < C_SwapchainImageCount; ++i) {
        m_presentSemaphores[i] = driver.m_device.createSemaphoreUnique(semaphoreinfo).value;
        m_renderSemaphores[i] = driver.m_device.createSemaphoreUnique(semaphoreinfo).value;
    }
}

vk::RenderPass Swapchain_Base::getCompositionRenderPass()
{
    return m_finalPass.get();
}

vk::Semaphore* Swapchain_Base::getPresentSemaphore()
{
    return &m_presentSemaphores[m_currentIndex].get();
}

vk::Semaphore* Swapchain_Base::getRenderSemaphore()
{
    return &m_renderSemaphores[m_currentIndex].get();
}

vk::Semaphore Swapchain_Base::getNextPresentSemaphore()
{
    return m_presentSemaphores[(m_currentIndex + 1) % C_SwapchainImageCount].get();
}

void Swapchain_Base::initFrameBuffers(std::vector<vk::Image>& _swapchainImages, uint2 _dims)
{
    Driver& driver = globals::getRef<Driver>();
    vk::Device device = driver.getDriverObjects().m_device;

    for (u32 i = 0; i < _swapchainImages.size(); ++i) {
        driver.nameImage(_swapchainImages[i], "Backbuffer image");

        vk::ImageViewCreateInfo viewinfo;
        viewinfo.setViewType(vk::ImageViewType::e2D);
        viewinfo.setImage(_swapchainImages[i]);
        viewinfo.setFormat(Swapchain::C_BackBufferFormat);
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

