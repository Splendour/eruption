#include "common.h"

#include "driver.h"
#include "GLFW/glfw3.h"
#include "window/window.h"

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

Driver::Driver(const class Window& _window)
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
        debuginfo.setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
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
        m_queueIndex = qindex;

        VERIFY_TRUE(m_gpu.getSurfaceSupportKHR(qindex, m_surface.get()).value);
    }
}


void Driver::nameImage(vk::Image _img, LiteralString _name)
{
    vk::DebugUtilsObjectNameInfoEXT info;
    info.setObjectType(vk::ObjectType::eImage);
    info.setObjectHandle((u64)_img.operator VkImage());
    info.setPObjectName(_name);
    VK_CHECK(m_device->setDebugUtilsObjectNameEXT(info, m_loader));
}


DriverObjects Driver::getDriverObjects()
{
    DriverObjects o;
    vk::Device d = m_device.get();
    o.m_device = d;
    o.m_gQueue = m_gQueue;
    o.m_instance = m_instance.get();
    o.m_surface = m_surface.get();
    o.m_queueIndex = m_queueIndex;
    return o;
}

vk::SurfaceCapabilitiesKHR Driver::getCaps()
{
    return m_gpu.getSurfaceCapabilitiesKHR(m_surface.get()).value;
}

void Driver::chooseAndInitGpu()
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
