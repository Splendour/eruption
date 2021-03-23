#include "common.h"

#include "swapchain_base.h"
#include "rendering/driver.h"
#include "globals.h"

Swapchain_Base::Swapchain_Base()
{
    vk::AttachmentDescription attachment;
    attachment.setFormat(C_BackBufferFormat);
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
}

vk::RenderPass Swapchain_Base::getCompositionRenderPass()
{
    return m_finalPass.get();
}


