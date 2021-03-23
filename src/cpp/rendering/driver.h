#pragma once

#include "platform/vk_common.h"

struct DriverObjects 
{
    vk::Instance m_instance;
    vk::Device m_device;
    vk::SurfaceKHR m_surface;
    vk::Queue m_gQueue;
    u32 m_queueIndex = 0;
};

class Driver 
{
public:
    Driver(const class Window& _window);

    DriverObjects getDriverObjects();
    void nameImage(vk::Image _img, LiteralString _name);

private:
    UniqueHandle<vk::Instance> m_instance;
    vk::DispatchLoaderDynamic m_loader;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;
    UniqueHandle<vk::SurfaceKHR> m_surface;
    UniqueHandle<vk::Device> m_device;
    vk::PhysicalDevice m_gpu;
    vk::Queue m_gQueue;
    u32 m_queueIndex;

    void chooseAndInitGpu();

};