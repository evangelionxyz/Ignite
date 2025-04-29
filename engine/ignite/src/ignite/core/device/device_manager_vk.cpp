#include "device_manager_vk.hpp"
#include "device_manager.hpp"

#include <string>

#include <GLFW/glfw3.h>

// Define the Vulkan dynamic dispatcher - this needs to occur in exactly one cpp file in the program.

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace ignite {
    static constexpr u32 kComputeQueueIndex = 0;
    static constexpr u32 kGraphicsQueueIndex = 0;
    static constexpr u32 kPresentQueueIndex = 0;
    static constexpr u32 kTransferQueueIndex = 0;

#define CHECK(a) if (!(a)) { return false; }

    static std::vector<const char *> StringSetToVector(const std::unordered_set<std::string> &set)
    {
        std::vector<const char *> ret;
        ret.reserve(set.size());
        for (const auto &s : set)
            ret.push_back(s.c_str());
        return ret;
    }

    template<typename T>
    static std::vector<T> SetToVector(const std::unordered_set<T> &set)
    {
        std::vector<T> ret;
        ret.reserve(set.size());
        for (const auto &s : set)
            ret.push_back(s);
        return ret;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugReportCallback(
        vk::DebugReportFlagsEXT flags,
        vk::DebugReportObjectTypeEXT objType,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char *layerPrefix,
        const char *msg,
        void *userData)
    {
        const DeviceManager_VK *manager = (const DeviceManager_VK *)userData;
        if (manager)
        {
            const auto &ignored = manager->GetDeviceParams().ignoreVulkanValidationMessageLocations;
            const auto found = std::find(ignored.begin(), ignored.end(), location);
            if (found != ignored.end())
                return VK_FALSE;
        }

        LOG_WARN("[VULKAN: Location={} code {}, layerPrefix='{}'] {}", location, code, layerPrefix, msg);

        return VK_FALSE;
    }

    static DeviceManager_VK *s_Instance = nullptr;

    DeviceManager *DeviceManager::CreateVK()
    {
        s_Instance = new DeviceManager_VK();
        return s_Instance;
    }

    DeviceManager_VK *DeviceManager_VK::GetInstance()
    {
        return s_Instance;
    }

    bool DeviceManager_VK::EnumerateAdapters(std::vector<AdapterInfo> &outAdapters)
    {
        if (!m_VulkanInstance)
            return false;

        std::vector<vk::PhysicalDevice> devices = m_VulkanInstance.enumeratePhysicalDevices();
        outAdapters.clear();

        for (auto physicalDevice : devices)
        {
            vk::PhysicalDeviceProperties2 properties2;
            vk::PhysicalDeviceIDProperties idProperties;
            properties2.pNext = &idProperties;
            physicalDevice.getProperties2(&properties2);

            auto const &properties = properties2.properties;

            AdapterInfo adapterInfo;
            adapterInfo.name = properties.deviceName.data();
            adapterInfo.vendorID = properties.vendorID;
            adapterInfo.deviceID = properties.deviceID;
            adapterInfo.vkPhysicalDevice = physicalDevice;
            adapterInfo.dedicatedVideoMemory = 0;

            std::array<uint8_t, 16> uuid;
            static_assert(uuid.size() == idProperties.deviceUUID.size());
            memcpy(uuid.data(), idProperties.deviceUUID.data(), uuid.size());
            adapterInfo.uuid = uuid;

            if (idProperties.deviceLUIDValid)
            {
                std::array<uint8_t, 8> luid;
                static_assert(luid.size() == idProperties.deviceLUID.size());
                memcpy(luid.data(), idProperties.deviceLUID.data(), luid.size());
                adapterInfo.luid = luid;
            }

            // Go through the memory types to figure out the amount of VRAM on this physical device.
            vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
            for (u32 heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex)
            {
                vk::MemoryHeap const &heap = memoryProperties.memoryHeaps[heapIndex];
                if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal)
                {
                    adapterInfo.dedicatedVideoMemory += heap.size;
                }
            }

            outAdapters.push_back(std::move(adapterInfo));
        }

        return true;
    }

    bool DeviceManager_VK::CreateInstanceInternal()
    {
        if (m_DeviceParams.enableDebugRuntime)
        {
            enabledExtensions.instance.insert("VK_EXT_debug_report");
            enabledExtensions.layers.insert("VK_LAYER_KHRONOS_validation");
        }

        m_DynamicLoader = std::make_unique<VulkanDynamicLoader>(m_DeviceParams.vulkanLibraryName);

        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_DynamicLoader->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        return CreateInstance();
    }

    bool DeviceManager_VK::CreateDevice()
    {
        if (m_DeviceParams.enableDebugRuntime)
        {
            InstallDebugCallback();
        }

        // add device extensions requested by the user
        for (const std::string &name : m_DeviceParams.requiredVulkanDeviceExtensions)
        {
            enabledExtensions.device.insert(name);
        }
        for (const std::string &name : m_DeviceParams.optionalVulkanDeviceExtensions)
        {
            optionalExtensions.device.insert(name);
        }

        if (!m_DeviceParams.headlessDevice)
        {
            // Need to adjust the swap chain format before creating the device because it affects physical device selection
            if (m_DeviceParams.swapChainFormat == nvrhi::Format::SRGBA8_UNORM)
                m_DeviceParams.swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
            else if (m_DeviceParams.swapChainFormat == nvrhi::Format::RGBA8_UNORM)
                m_DeviceParams.swapChainFormat = nvrhi::Format::BGRA8_UNORM;

            CHECK(CreateWindowSurface())
        }

        CHECK(PickPhysicalDevice())
        CHECK(FindQueueFamilies(m_VulkanPhysicalDevice))
        CHECK(CreateVkDevice())

        auto vecInstanceExt = StringSetToVector(enabledExtensions.instance);
        auto vecLayers = StringSetToVector(enabledExtensions.layers);
        auto vecDeviceExt = StringSetToVector(enabledExtensions.device);

        nvrhi::vulkan::DeviceDesc deviceDesc;
        deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
        deviceDesc.instance = m_VulkanInstance;
        deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
        deviceDesc.device = m_VulkanDevice;
        deviceDesc.graphicsQueue = m_GraphicsQueue;
        deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;

        if (m_DeviceParams.enableComputeQueue)
        {
            deviceDesc.computeQueue = m_ComputeQueue;
            deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
        }

        if (m_DeviceParams.enableCopyQueue)
        {
            deviceDesc.transferQueue = m_TransferQueue;
            deviceDesc.transferQueueIndex = m_TransferQueueFamily;
        }

        deviceDesc.instanceExtensions = vecInstanceExt.data();
        deviceDesc.numInstanceExtensions = vecInstanceExt.size();
        deviceDesc.deviceExtensions = vecDeviceExt.data();
        deviceDesc.numDeviceExtensions = vecDeviceExt.size();
        deviceDesc.bufferDeviceAddressSupported = m_BufferDeviceAddressSupported;
        deviceDesc.vulkanLibraryName = m_DeviceParams.vulkanLibraryName;

        m_NvrhiDevice = nvrhi::vulkan::createDevice(deviceDesc);

        if (m_DeviceParams.enableNvrhiValidationLayer)
        {
            m_ValidationLayer = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
        }

        return true;
    }

    bool DeviceManager_VK::CreateSwapChain()
    {
       CHECK(CreateVkSwapChain())

       m_PresentSemaphores.reserve(m_DeviceParams.maxFramesInFlight + 1);
       m_AcquireSemaphores.reserve(m_DeviceParams.maxFramesInFlight + 1);
       for (uint32_t i = 0; i < m_DeviceParams.maxFramesInFlight + 1; ++i)
       {
           m_PresentSemaphores.push_back(m_VulkanDevice.createSemaphore(vk::SemaphoreCreateInfo()));
           m_AcquireSemaphores.push_back(m_VulkanDevice.createSemaphore(vk::SemaphoreCreateInfo()));
       }

       return true;
    }

    void DeviceManager_VK::DestroyDeviceAndSwapChain()
    {
        DestroySwapChain();

        for (auto &semaphore : m_PresentSemaphores)
        {
            if (semaphore)
            {
                m_VulkanDevice.destroySemaphore(semaphore);
                semaphore = nullptr;
            }
        }

        for (auto &semaphore : m_AcquireSemaphores)
        {
            if (semaphore)
            {
                m_VulkanDevice.destroySemaphore(semaphore);
                semaphore = nullptr;
            }
        }
        
        m_RendererString.clear();

        if (m_WindowSurface)
        {
            assert(m_VulkanInstance);
            m_VulkanInstance.destroySurfaceKHR(m_WindowSurface);
            m_WindowSurface = nullptr;
        }

        if (m_DebugReportCallback)
        {
            m_VulkanInstance.destroyDebugReportCallbackEXT(m_DebugReportCallback);
        }

        m_ValidationLayer = nullptr;
        
        m_NvrhiDevice->waitForIdle();
        m_NvrhiDevice->runGarbageCollection();

        if (m_VulkanDevice)
        {
            m_VulkanDevice.destroy();
            m_VulkanDevice = nullptr;
        }

        if (m_VulkanInstance)
        {
            m_VulkanInstance.destroy();
            m_VulkanInstance = nullptr;
        }
    }

    void DeviceManager_VK::ResizeSwapChain()
    {
        if (m_VulkanDevice)
        {
            DestroySwapChain();
            CreateSwapChain();
        }
    }

    nvrhi::ITexture *DeviceManager_VK::GetCurrentBackBuffer()
    {
        return m_SwapChainImages[m_SwapChainIndex].rhiHandle;
    }

    nvrhi::ITexture *DeviceManager_VK::GetBackBuffer(u32 index)
    {
        if (index < m_SwapChainImages.size())
            return m_SwapChainImages[index].rhiHandle;
        return nullptr;
    }

    u32 DeviceManager_VK::GetCurrentBackBufferIndex()
    {
        return m_SwapChainIndex;
    }

    u32 DeviceManager_VK::GetBackBufferCount()
    {
        return uint32_t(m_SwapChainImages.size());
    }

    bool DeviceManager_VK::BeginFrame()
    {
        const auto &semaphore = m_AcquireSemaphores[m_AcquireSemaphoreIndex];

        vk::Result res;

        int const maxAttempts = 3;
        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            res = m_VulkanDevice.acquireNextImageKHR(
                m_SwapChain,
                std::numeric_limits<uint64_t>::max(), // timeout
                semaphore,
                vk::Fence(),
                &m_SwapChainIndex);

            if (res == vk::Result::eErrorOutOfDateKHR && attempt < maxAttempts)
            {
                auto surfaceCaps = m_VulkanPhysicalDevice.getSurfaceCapabilitiesKHR(m_WindowSurface);

                m_DeviceParams.backBufferWidth = surfaceCaps.currentExtent.width;
                m_DeviceParams.backBufferHeight = surfaceCaps.currentExtent.height;

                ResizeSwapChain();
                CreateBackBuffers();
            }
            else
                break;
        }

        m_AcquireSemaphoreIndex = (m_AcquireSemaphoreIndex + 1) % m_AcquireSemaphores.size();

        if (res == vk::Result::eSuccess)
        {
            // Schedule the wait. The actual wait operation will be submitted when the app executes any command list.
            m_NvrhiDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);
            return true;
        }

        return false;
    }

    bool DeviceManager_VK::Present()
    {
        const auto &semaphore = m_PresentSemaphores[m_PresentSemaphoreIndex];

        m_NvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);

        // NVRHI buffers the semaphores and signals them when something is submitted to a queue.
        // Call 'executeCommandLists' with no command lists to actually signal the semaphore.
        m_NvrhiDevice->executeCommandLists(nullptr, 0);

        vk::PresentInfoKHR info = vk::PresentInfoKHR()
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&semaphore)
            .setSwapchainCount(1)
            .setPSwapchains(&m_SwapChain)
            .setPImageIndices(&m_SwapChainIndex);

        const vk::Result res = m_PresentQueue.presentKHR(&info);
        if (!(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR))
        {
            return false;
        }

        m_PresentSemaphoreIndex = (m_PresentSemaphoreIndex + 1) % m_PresentSemaphores.size();

#ifndef PLATFORM_WINDOWS
        if (m_DeviceParams.vsyncEnable || m_DeviceParams.enableDebugRuntime)
        {
            // according to vulkan-tutorial.com, "the validation layer implementation expects
            // the application to explicitly synchronize with the GPU"
            m_PresentQueue.waitIdle();
        }
#endif

        while (m_FramesInFlight.size() >= m_DeviceParams.maxFramesInFlight)
        {
            auto query = m_FramesInFlight.front();
            m_FramesInFlight.pop();

            m_NvrhiDevice->waitEventQuery(query);

            m_QueryPool.push_back(query);
        }

        nvrhi::EventQueryHandle query;
        if (!m_QueryPool.empty())
        {
            query = m_QueryPool.back();
            m_QueryPool.pop_back();
        }
        else
        {
            query = m_NvrhiDevice->createEventQuery();
        }

        m_NvrhiDevice->resetEventQuery(query);
        m_NvrhiDevice->setEventQuery(query, nvrhi::CommandQueue::Graphics);
        m_FramesInFlight.push(query);

        return true;
    }

    const char *DeviceManager_VK::GetRendererString() const
    {
        return m_RendererString.c_str();
    }

    bool DeviceManager_VK::IsVulkanInstanceExtensionEnabled(const char *extensionName) const
    {
        return enabledExtensions.instance.contains(extensionName);
    }

    bool DeviceManager_VK::IsVulkanDeviceExtensionEnabled(const char *extensionName) const
    {
        return enabledExtensions.device.contains(extensionName);
    }

    bool DeviceManager_VK::IsVulkanLayerEnabled(const char *layerName) const
    {
        return enabledExtensions.layers.contains(layerName);
    }

    void DeviceManager_VK::GetEnabledVulkanInstanceExtensions(std::vector<std::string> &extensions) const
    {
        for (const auto &ext : enabledExtensions.instance)
            extensions.push_back(ext);
    }

    void DeviceManager_VK::GetEnabledVulkanDeviceExtensions(std::vector<std::string> &extensions) const
    {
        for (const auto &ext : enabledExtensions.device)
            extensions.push_back(ext);
    }

    void DeviceManager_VK::GetEnabledVulkanLayers(std::vector<std::string> &layers) const
    {
        for (const auto &ext : enabledExtensions.layers)
            layers.push_back(ext);
    }

    bool DeviceManager_VK::CreateInstance()
    {
        if (!m_DeviceParams.headlessDevice)
        {
            if (!glfwVulkanSupported())
            {
                LOG_ERROR("GLFW reports that Vulkan is not supported. Perhaps missing a call to glfwInit()");
                return false;
            }

            u32 glfwExtCount;
            const char **glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);
            LOG_ASSERT(glfwExt, " Failed to get required instance extensions");

            for (u32 i = 0; i < glfwExtCount; ++i)
            {
                enabledExtensions.instance.insert(std::string(glfwExt[i]));
            }
        }

        // add instance extensions requested by the user
        for (const std::string &name : m_DeviceParams.requiredVulkanInstanceExtensions)
        {
            enabledExtensions.instance.insert(name);
        }

        for (const std::string &name : m_DeviceParams.optionalVulaknInstanceExtensions)
        {
            optionalExtensions.instance.insert(name);
        }

        // add layers requested by the user
        for (const std::string &name : m_DeviceParams.requiredVulkanLayers)
        {
            enabledExtensions.layers.insert(name);
        }

        for (const std::string &name : m_DeviceParams.optionalVulkanLayers)
        {
            optionalExtensions.layers.insert(name);
        }

        std::unordered_set<std::string> requiredExtensions = enabledExtensions.instance;

        // figure out which optional extensions are supported
        for (const auto &instanceExt : vk::enumerateInstanceExtensionProperties())
        {
            const std::string name = instanceExt.extensionName;
            if (optionalExtensions.instance.contains(name))
                enabledExtensions.instance.insert(name);

            requiredExtensions.erase(name);
        }

        if (!requiredExtensions.empty())
        {
            std::stringstream ss;
            ss << "Cannot create a Vulkan instance because the following required extension(s) are note supported: ";
            for (const auto &ext : requiredExtensions)
            {
                ss << '\n' << ' - ' << ext;
            }

            LOG_ASSERT(false, "{}", ss.str());
            return false;
        }

        LOG_INFO("Enabled Vulkan instance extensions: ");
        for (const auto &ext : enabledExtensions.instance)
        {
            LOG_INFO("    {}", ext);
        }

        std::unordered_set<std::string> requiredLayers = enabledExtensions.layers;
        for (const auto &layer : vk::enumerateInstanceLayerProperties())
        {
            const std::string name = layer.layerName;
            LOG_WARN("{}", name);
            if (optionalExtensions.layers.contains(name))
                enabledExtensions.layers.insert(name);

            requiredLayers.erase(name);
        }

        if (!requiredLayers.empty())
        {
            std::stringstream ss;
            ss << "Cannot create a Vulkan instance because the following required layer(s) are not support: ";
            for (const auto &ext : requiredLayers)
                ss << '\n' << " - " << ext;
            LOG_ASSERT(false, "{}", ss.str());
            return false;
        }

        LOG_INFO("Enabled Vulkan layers: ");
        for (const auto &layer : enabledExtensions.layers)
        {
            LOG_INFO("    {}", layer.c_str());
        }

        auto instanceExtVec = StringSetToVector(enabledExtensions.instance);
        auto layerVec = StringSetToVector(enabledExtensions.layers);

        auto applicationInfo = vk::ApplicationInfo();

        vk::Result res = vk::enumerateInstanceVersion(&applicationInfo.apiVersion);

        // query the Vulkan API version supported on the system to make sure we use at lease 1.3 when that's present.

        if (res != vk::Result::eSuccess)
        {
            std::string resultStr = nvrhi::vulkan::resultToString(VkResult(res));
            LOG_ASSERT(false, "Call to vkEnumerateInstanceVerion failed, error code {}", resultStr);
            return false;
        }

        const u32 minimumVulkanVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        if (applicationInfo.apiVersion < minimumVulkanVersion)
        {
            LOG_ASSERT(false, "The Vulkan API version supported on the system ({}.{}.{}) is too low, at least {}.{}.{} is required",
                VK_API_VERSION_MAJOR(applicationInfo.apiVersion), VK_API_VERSION_MINOR(applicationInfo.apiVersion), VK_API_VERSION_PATCH(applicationInfo.apiVersion), VK_API_VERSION_MAJOR(minimumVulkanVersion), VK_API_VERSION_MINOR(minimumVulkanVersion), VK_API_VERSION_PATCH(minimumVulkanVersion)
                );
            return false;
        }

        // spec says: a non zero variant indicates the API is a variant of the Vulkan API and applications will typically need to modified to run against it.
        if (VK_API_VERSION_VARIANT(applicationInfo.apiVersion) != 0)
        {
            LOG_ASSERT(false, "The Vulkan API supported on the system yses an unexpected variant: {}", VK_API_VERSION_VARIANT(applicationInfo.apiVersion));
            return false;
        }

        // create the Vulkan Instance
        vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
            .setEnabledLayerCount(u32(layerVec.size()))
            .setPpEnabledLayerNames(layerVec.data())
            .setEnabledExtensionCount(u32(instanceExtVec.size()))
            .setPpEnabledExtensionNames(instanceExtVec.data())
            .setPApplicationInfo(&applicationInfo);

        res = vk::createInstance(&info, nullptr, &m_VulkanInstance);
        if (res != vk::Result::eSuccess)
        {
            std::string resultStr = nvrhi::vulkan::resultToString(VkResult(res));
            LOG_ASSERT(false, "Failed to create a Vulkan instance, error code: {}", resultStr);
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanInstance);

        return true;
    }

    bool DeviceManager_VK::CreateWindowSurface()
    {
        VkResult res = glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, (VkSurfaceKHR *)&m_WindowSurface);
        LOG_ASSERT(res == VK_SUCCESS, "Failed to create GLFW window surface, error code: {}", nvrhi::vulkan::resultToString(res));
        return true;
    }

    void DeviceManager_VK::InstallDebugCallback()
    {
        const auto info = vk::DebugReportCallbackCreateInfoEXT()
            .setFlags(vk::DebugReportFlagBitsEXT::eError
            | vk::DebugReportFlagBitsEXT::eWarning
            //| vk::DebugReportFlagBitsEXT::eInformation
            | vk::DebugReportFlagBitsEXT::ePerformanceWarning)
            .setPfnCallback(VkDebugReportCallback)
            .setPUserData(this);

        vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT(&info, nullptr, &m_DebugReportCallback);

        LOG_ASSERT(res == vk::Result::eSuccess, "Vulkan: Failed to create report debug callback");
    }

    bool DeviceManager_VK::PickPhysicalDevice()
    {
        VkFormat requestedFormat = nvrhi::vulkan::convertFormat(m_DeviceParams.swapChainFormat);
        vk::Extent2D requestedExtent(m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight);

        auto devices = m_VulkanInstance.enumeratePhysicalDevices();

        i32 adapterIndex = m_DeviceParams.adapterIndex;

        i32 firstDevice = 0;
        i32 lastDevice = i32(devices.size()) - 1;

        if (adapterIndex >= 0)
        {
            LOG_ASSERT(adapterIndex <= lastDevice, "The specified Vulkan physical device {} does note exists.", adapterIndex);

            firstDevice = adapterIndex;
            lastDevice = adapterIndex;
        }

        std::stringstream errorStream;
        errorStream << "Cannot find a Vulkan device that supports all the required extensions and properties";

        // build a list of GPUs
        std::vector<vk::PhysicalDevice> discreteGPUs;
        std::vector<vk::PhysicalDevice> otherGPUs;
        for (i32 deviceIndex = firstDevice; deviceIndex <= lastDevice; ++deviceIndex)
        {
            const vk::PhysicalDevice &dev = devices[deviceIndex];
            vk::PhysicalDeviceProperties prop = dev.getProperties();

            errorStream << '\n' << prop.deviceName.data() << ':';

            // check all required extensions are present
            std::unordered_set<std::string> requiredExtensions = enabledExtensions.device;
            auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
            for (const auto &ext : deviceExtensions)
            {
                requiredExtensions.erase(std::string(ext.extensionName.data()));
            }

            bool deviceIsGood = true;

            if (!requiredExtensions.empty())
            {
                // device is missing one or more required extensions
                for (const auto &ext : requiredExtensions)
                {
                    errorStream << '\n' << "  - missing " << ext;
                }
                deviceIsGood = false;
            }

            auto deviceFeatures = dev.getFeatures();
            if (!deviceFeatures.samplerAnisotropy)
            {
                // device is toaster oven
                errorStream << '\n' << "  - does not support samplerAnisotropy";
                deviceIsGood = false;
            }

            if (!deviceFeatures.textureCompressionBC)
            {
                errorStream << '\n' << "  - does not support textureCompressionBC";
                deviceIsGood = false;
            }

            if (!FindQueueFamilies(dev))
            {
                // device does not have all the queue families we need
                errorStream << '\n' << "  - does note support the necessary queue types";
                deviceIsGood = false;
            }

            if (deviceIsGood && m_WindowSurface)
            {
                bool surfaceSupported = dev.getSurfaceSupportKHR(m_PresentQueueFamily, m_WindowSurface);
                if (!surfaceSupported)
                {
                    errorStream << '\n' << "  - does not support the window surface";
                    deviceIsGood = false;
                }
                else
                {
                    // check that this device supports out intended swap chain creation parameters
                    auto surfaceCaps = dev.getSurfaceCapabilitiesKHR(m_WindowSurface);
                    auto surfaceFmts = dev.getSurfaceFormatsKHR(m_WindowSurface);

                    if (surfaceCaps.minImageCount > m_DeviceParams.swapChainBufferCount || (surfaceCaps.maxImageCount < m_DeviceParams.swapChainBufferCount && surfaceCaps.maxImageCount > 0))
                    {
                        errorStream << '\n' << "  - cannot support the requested swap chain image count:";
                        errorStream << " requested " << m_DeviceParams.swapChainBufferCount << ", available " << surfaceCaps.minImageCount << " - " << surfaceCaps.maxImageCount;

                        deviceIsGood = false;
                    }

                    if (surfaceCaps.minImageExtent.width > requestedExtent.width || surfaceCaps.minImageExtent.height > requestedExtent.height || surfaceCaps.maxImageExtent.width < requestedExtent.width || surfaceCaps.maxImageExtent.height < requestedExtent.height)
                    {
                        errorStream << '\n' << "  - cannot support requested swap chain image size:";
                        errorStream << " requested " << requestedExtent.width << "x" << requestedExtent.height << ", ";
                        errorStream << " available " << surfaceCaps.minImageExtent.width << "x" << surfaceCaps.minImageExtent.height;
                        errorStream << " - " << surfaceCaps.maxImageExtent.width << "x" << surfaceCaps.maxImageExtent.height;
                        deviceIsGood = false;
                    }

                    bool surfaceFormatPresent = false;
                    for (const vk::SurfaceFormatKHR &surfaceFmt : surfaceFmts)
                    {
                        if (surfaceFmt.format == vk::Format(requestedFormat))
                        {
                            surfaceFormatPresent = true;
                            break;
                        }
                    }

                    if (!surfaceFormatPresent)
                    {
                        // can't create a swapchain using the format requested
                        errorStream << '\n' << "  - does not support the requested swap chain format";
                        deviceIsGood = false;
                    }

                    u32 canPresent = dev.getSurfaceSupportKHR(m_GraphicsQueueFamily, m_WindowSurface);
                    if (!canPresent)
                    {
                        errorStream << '\n' << "  - cannot present";
                        deviceIsGood = false;
                    }
                }
            }

            if (!deviceIsGood)
                continue;

            if (prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                discreteGPUs.push_back(dev);
            }
            else
            {
                otherGPUs.push_back(dev);
            }
        }

        // pick up first discrete GPU if it exists, otherwise the first integrated GPU
        if (!discreteGPUs.empty())
        {
            u32 selectedIndex = 0;
            m_VulkanPhysicalDevice = discreteGPUs[selectedIndex];

            return true;
        }

        if (!otherGPUs.empty())
        {
            u32 selectedIndex = 0;
            m_VulkanPhysicalDevice = otherGPUs[selectedIndex];

            return true;
        }

        LOG_ERROR("{}", errorStream.str());
        return false;
    }

    bool DeviceManager_VK::FindQueueFamilies(vk::PhysicalDevice physicalDevice)
    {
        auto props = physicalDevice.getQueueFamilyProperties();

        for (int i = 0; i < int(props.size()); i++)
        {
            const auto &queueFamily = props[i];

            if (m_GraphicsQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_GraphicsQueueFamily = i;
                }
            }

            if (m_ComputeQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_ComputeQueueFamily = i;
                }
            }

            if (m_TransferQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_TransferQueueFamily = i;
                }
            }

            if (m_PresentQueueFamily == -1)
            {
                if (queueFamily.queueCount > 0 &&
                    glfwGetPhysicalDevicePresentationSupport(m_VulkanInstance, physicalDevice, i))
                {
                    m_PresentQueueFamily = i;
                }
            }
        }

        if (m_GraphicsQueueFamily == -1 ||
            (m_PresentQueueFamily == -1 && !m_DeviceParams.headlessDevice) ||
            (m_ComputeQueueFamily == -1 && m_DeviceParams.enableComputeQueue) ||
            (m_TransferQueueFamily == -1 && m_DeviceParams.enableCopyQueue))
        {
            return false;
        }

        return true;
    }

    bool DeviceManager_VK::CreateVkDevice()
    {
        // figure out which optional extensions are supported
        auto deviceExtensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
        for (const auto &ext : deviceExtensions)
        {
            const std::string name = ext.extensionName;
            if (optionalExtensions.device.contains(name))
            {
                if (name == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME && m_DeviceParams.headlessDevice)
                    continue;

                enabledExtensions.device.insert(name);
            }

            if (m_DeviceParams.enableRayTracingExtensions && m_RayTracingExtensions.contains(name))
            {
                enabledExtensions.device.insert(name);
            }
        }

        if (!m_DeviceParams.headlessDevice)
        {
            enabledExtensions.device.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        const vk::PhysicalDeviceProperties physicalDeviceProperties = m_VulkanPhysicalDevice.getProperties();
        m_RendererString = std::string(physicalDeviceProperties.deviceName.data());

        bool accelStructSupported = false;
        bool rayPipelineSupported = false;
        bool rayQuerySupported = false;
        bool meshletsSupported = false;
        bool vrsSupported = false;
        bool interlockSupported = false;
        bool barycentricSupported = false;
        bool storage16BitSupported = false;
        bool synchronization2Supported = false;
        bool maintenance4Supported = false;
        bool aftermathSupported = false;

        LOG_INFO("Enabled Vulkan device extensions: ");
        for (const auto &ext : enabledExtensions.device)
        {
            LOG_INFO("   {}", ext.c_str());

            if (ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                accelStructSupported = true;
            else if (ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                rayPipelineSupported = true;
            else if (ext == VK_KHR_RAY_QUERY_EXTENSION_NAME)
                rayQuerySupported = true;
            else if (ext == VK_NV_MESH_SHADER_EXTENSION_NAME)
                meshletsSupported = true;
            else if (ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)
                vrsSupported = true;
            else if (ext == VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME)
                interlockSupported = true;
            else if (ext == VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME)
                barycentricSupported = true;
            else if (ext == VK_KHR_16BIT_STORAGE_EXTENSION_NAME)
                storage16BitSupported = true;
            else if (ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                synchronization2Supported = true;
            else if (ext == VK_KHR_MAINTENANCE_4_EXTENSION_NAME)
                maintenance4Supported = true;
            else if (ext == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME)
                m_SwapChainMutableFormatSupported = true;
            else if (ext == VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME)
                aftermathSupported = true;
        }

#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }
        void *pNext = nullptr;

        vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
        // determine support for buffer device address, the Vulkan 1.2 way
        auto bufferDeviceAddressFeatures = vk::PhysicalDeviceBufferDeviceAddressFeatures();
        // determine support for maintenance4
        auto maintenance4Features = vk::PhysicalDeviceMaintenance4Features();
        // determine support for aftermath
        auto aftermathPhysicalFeatures  = vk::PhysicalDeviceDiagnosticsConfigFeaturesNV();

        // put the user provided extension structure at the end of the chain
        pNext = m_DeviceParams.physicalDeviceFeatures2Extensions;
        APPEND_EXTENSION(true, bufferDeviceAddressFeatures);
        APPEND_EXTENSION(maintenance4Supported, maintenance4Features);
        APPEND_EXTENSION(aftermathSupported, aftermathPhysicalFeatures);

        physicalDeviceFeatures2.pNext = pNext;
        m_VulkanPhysicalDevice.getFeatures2(&physicalDeviceFeatures2);

        std::unordered_set<int> uniqueQueueFamilies = {
            m_GraphicsQueueFamily
        };

        if (!m_DeviceParams.headlessDevice)
            uniqueQueueFamilies.insert(m_PresentQueueFamily);
        
        if (m_DeviceParams.enableComputeQueue)
            uniqueQueueFamilies.insert(m_ComputeQueueFamily);

        if (m_DeviceParams.enableCopyQueue)
            uniqueQueueFamilies.insert(m_TransferQueueFamily);

        f32 priority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> queueDesc;
        queueDesc.reserve(uniqueQueueFamilies.size());

        for (i32 queueFamily : uniqueQueueFamilies)
        {
            queueDesc.push_back(vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(queueFamily)
                .setQueueCount(1)
                .setPQueuePriorities(&priority));
        }

        auto accelStructFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
            .setAccelerationStructure(true);
        auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
            .setRayTracingPipeline(true)
            .setRayTraversalPrimitiveCulling(true);
        auto rayQueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR()
            .setRayQuery(true);
        auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
            .setTaskShader(true)
            .setMeshShader(true);
        auto interlockFeatures = vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT()
            .setFragmentShaderPixelInterlock(true);
        auto barycentricFeatures = vk::PhysicalDeviceFragmentShaderBarycentricFeaturesKHR()
            .setFragmentShaderBarycentric(true);
        auto storage16BitFeatures = vk::PhysicalDevice16BitStorageFeatures()
            .setStorageBuffer16BitAccess(true);
        auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
            .setPipelineFragmentShadingRate(true)
            .setPrimitiveFragmentShadingRate(true)
            .setAttachmentFragmentShadingRate(true);
        auto vulkan13features = vk::PhysicalDeviceVulkan13Features()
            .setSynchronization2(synchronization2Supported)
            .setMaintenance4(maintenance4Features.maintenance4);
        auto aftermathFeatures = vk::DeviceDiagnosticsConfigCreateInfoNV()
            .setFlags(vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking
            | vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo
            | vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderErrorReporting);

        pNext = nullptr;
        APPEND_EXTENSION(accelStructSupported, accelStructFeatures)
        APPEND_EXTENSION(rayPipelineSupported, rayPipelineFeatures)
        APPEND_EXTENSION(rayQuerySupported, rayQueryFeatures)
        APPEND_EXTENSION(meshletsSupported, meshletFeatures)
        APPEND_EXTENSION(vrsSupported, vrsFeatures)
        APPEND_EXTENSION(interlockSupported, interlockFeatures)
        APPEND_EXTENSION(barycentricSupported, barycentricFeatures)
        APPEND_EXTENSION(storage16BitSupported, storage16BitFeatures)
        APPEND_EXTENSION(physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3, vulkan13features)
        APPEND_EXTENSION(physicalDeviceProperties.apiVersion < VK_API_VERSION_1_3 &&maintenance4Supported, maintenance4Features);

#undef APPEND_EXTENSION

        auto deviceFeatures = vk::PhysicalDeviceFeatures()
            .setShaderImageGatherExtended(true)
            .setSamplerAnisotropy(true)
            .setTessellationShader(true)
            .setTextureCompressionBC(true)
            .setGeometryShader(true)
            .setImageCubeArray(true)
            .setShaderInt16(true)
            .setFillModeNonSolid(true)
            .setFragmentStoresAndAtomics(true)
            .setDualSrcBlend(true)
            .setVertexPipelineStoresAndAtomics(true);

        // add a Vulkan 1.1 structure with default settings to make it easier for apps to modify them
        auto vulkan11features = vk::PhysicalDeviceVulkan11Features()
            .setPNext(pNext);

        auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
            .setDescriptorIndexing(true)
            .setRuntimeDescriptorArray(true)
            .setDescriptorBindingPartiallyBound(true)
            .setDescriptorBindingVariableDescriptorCount(true)
            .setTimelineSemaphore(true)
            .setShaderSampledImageArrayNonUniformIndexing(true)
            .setBufferDeviceAddress(bufferDeviceAddressFeatures.bufferDeviceAddress)
            .setPNext(&vulkan11features);

        auto layerVec = StringSetToVector(enabledExtensions.layers);
        auto extVec = StringSetToVector(enabledExtensions.device);

        auto deviceDesc = vk::DeviceCreateInfo()
            .setPQueueCreateInfos(queueDesc.data())
            .setQueueCreateInfoCount(uint32_t(queueDesc.size()))
            .setPEnabledFeatures(&deviceFeatures)
            .setEnabledExtensionCount(uint32_t(extVec.size()))
            .setPpEnabledExtensionNames(extVec.data())
            .setEnabledLayerCount(uint32_t(layerVec.size()))
            .setPpEnabledLayerNames(layerVec.data())
            .setPNext(&vulkan12features);

        if (m_DeviceParams.deviceCreateInfoCallback)
            m_DeviceParams.deviceCreateInfoCallback(deviceDesc);

        const vk::Result res = m_VulkanPhysicalDevice.createDevice(&deviceDesc, nullptr, &m_VulkanDevice);
        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan physical device, error code: {}", nvrhi::vulkan::resultToString(VkResult(res)));

            return false;
        }

        m_VulkanDevice.getQueue(m_GraphicsQueueFamily, kGraphicsQueueIndex, &m_GraphicsQueue);
        if (m_DeviceParams.enableComputeQueue)
            m_VulkanDevice.getQueue(m_ComputeQueueFamily, kComputeQueueIndex, &m_ComputeQueue);
        if (m_DeviceParams.enableCopyQueue)
            m_VulkanDevice.getQueue(m_TransferQueueFamily, kTransferQueueIndex, &m_TransferQueue);
        if (!m_DeviceParams.headlessDevice)
            m_VulkanDevice.getQueue(m_PresentQueueFamily, kPresentQueueIndex, &m_PresentQueue);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanDevice);

        // remember the bufferDeviceAddress feature enable
        m_BufferDeviceAddressSupported = vulkan12features.bufferDeviceAddress;

        LOG_INFO("Created Vulkan device: {}", m_RendererString.c_str());

        return true;
    }

    bool DeviceManager_VK::CreateVkSwapChain()
    {
        DestroySwapChain();

        m_SwapChainFormat = {
            vk::Format(nvrhi::vulkan::convertFormat(m_DeviceParams.swapChainFormat)),
            vk::ColorSpaceKHR::eSrgbNonlinear
        };

        vk::Extent2D extent = vk::Extent2D(m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight);

        std::unordered_set<u32> uniqueQueues = {
            u32(m_GraphicsQueueFamily),
            u32(m_PresentQueueFamily)
        };

        std::vector<u32> queues = SetToVector(uniqueQueues);

        const bool enableSwapChainSharing = queues.size() > 1;

        auto desc = vk::SwapchainCreateInfoKHR()
            .setSurface(m_WindowSurface)
            .setMinImageCount(m_DeviceParams.swapChainBufferCount)
            .setImageFormat(m_SwapChainFormat.format)
            .setImageColorSpace(m_SwapChainFormat.colorSpace)
            .setImageExtent(extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setImageSharingMode(enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
            .setFlags(m_SwapChainMutableFormatSupported ? vk::SwapchainCreateFlagBitsKHR::eMutableFormat : vk::SwapchainCreateFlagBitsKHR(0))
            .setQueueFamilyIndexCount(enableSwapChainSharing ? uint32_t(queues.size()) : 0)
            .setPQueueFamilyIndices(enableSwapChainSharing ? queues.data() : nullptr)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(m_DeviceParams.vsyncEnable ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate)
            .setClipped(true)
            .setOldSwapchain(nullptr);

        std::vector<vk::Format> imageFormats = { m_SwapChainFormat.format };
        switch (m_SwapChainFormat.format)
        {
            case vk::Format::eR8G8B8A8Unorm:
                imageFormats.push_back(vk::Format::eR8G8B8A8Srgb);
                break;
            case vk::Format::eR8G8B8A8Srgb:
                imageFormats.push_back(vk::Format::eR8G8B8A8Unorm);
                break;
            case vk::Format::eB8G8R8A8Unorm:
                imageFormats.push_back(vk::Format::eB8G8R8A8Srgb);
                break;
            case vk::Format::eB8G8R8A8Srgb:
                imageFormats.push_back(vk::Format::eB8G8R8A8Unorm);
                break;
            default:
                break;
        }

        auto imageFormatListCreateInfo = vk::ImageFormatListCreateInfo()
            .setViewFormats(imageFormats);

        if (m_SwapChainMutableFormatSupported)
            desc.pNext = &imageFormatListCreateInfo;

        const vk::Result res = m_VulkanDevice.createSwapchainKHR(&desc, nullptr, &m_SwapChain);

        LOG_ASSERT(res == vk::Result::eSuccess, "Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString(VkResult(res)));


        // retrieve swap chain images
        auto images = m_VulkanDevice.getSwapchainImagesKHR(m_SwapChain);
        for (auto image : images)
        {
            SwapChainImage sci;
            sci.image = image;

            nvrhi::TextureDesc textureDesc;
            textureDesc.width = m_DeviceParams.backBufferWidth;
            textureDesc.height = m_DeviceParams.backBufferHeight;
            textureDesc.format = m_DeviceParams.swapChainFormat;
            textureDesc.debugName = "Swap chain image";
            textureDesc.initialState = nvrhi::ResourceStates::Present;
            textureDesc.keepInitialState = true;
            textureDesc.isRenderTarget = true;

            sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.image), textureDesc);
            m_SwapChainImages.push_back(sci);
        }

        m_SwapChainIndex = 0;

        return true;
    }

    void DeviceManager_VK::DestroySwapChain()
    {
        if (m_VulkanDevice)
        {
            m_VulkanDevice.waitIdle();
        }

        if (m_SwapChain)
        {
            m_VulkanDevice.destroySwapchainKHR(m_SwapChain);
            m_SwapChain = nullptr;
        }

        m_SwapChainImages.clear();
    }

    void DeviceManager_VK::WaitForIdle()
    {
        if (m_VulkanDevice)
        {
            m_VulkanDevice.waitIdle();
        }
    }

}