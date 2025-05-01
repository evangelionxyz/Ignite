#pragma once

#include <string>
#include <queue>
#include <unordered_set>

#include "device_manager.hpp"

#include "ignite/core/logger.hpp"

#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>

#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace ignite
{
    class DeviceManager_VK : public DeviceManager
    {
    public:
        [[nodiscard]] nvrhi::IDevice *GetDevice() const override
        {
            if (m_ValidationLayer)
                return m_ValidationLayer;

            return m_NvrhiDevice;
        }

        [[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const override
        {
            return nvrhi::GraphicsAPI::VULKAN;
        }

        static DeviceManager_VK *GetInstance();

        bool EnumerateAdapters(std::vector<AdapterInfo> &outAdapters) override;
        const DeviceCreationParameters &GetDeviceParams() const { return m_DeviceParams; }

        bool CreateInstanceInternal() override;
        bool CreateDevice() override;
        bool CreateSwapChain() override;
        void DestroyDeviceAndSwapChain() override;

        void ResizeSwapChain() override;

        nvrhi::ITexture *GetCurrentBackBuffer() override;
        nvrhi::ITexture *GetBackBuffer(u32 index) override;
        u32 GetCurrentBackBufferIndex() override;
        u32 GetBackBufferCount() override;

        bool BeginFrame() override;
        bool Present() override;

        const char *GetRendererString() const override;
        bool IsVulkanInstanceExtensionEnabled(const char *extensionName) const override;
        bool IsVulkanDeviceExtensionEnabled(const char *extensionName) const override;
        bool IsVulkanLayerEnabled(const char *layerName) const override;

        void GetEnabledVulkanInstanceExtensions(std::vector<std::string> &extensions) const override;
        void GetEnabledVulkanDeviceExtensions(std::vector<std::string> &extensions) const override;
        void GetEnabledVulkanLayers(std::vector<std::string> &layers) const override;

        bool CreateInstance();
        bool CreateWindowSurface();
        void InstallDebugCallback();
        bool PickPhysicalDevice();
        bool FindQueueFamilies(vk::PhysicalDevice physicalDevice);
        bool CreateVkDevice();
        bool CreateVkSwapChain();
        void DestroySwapChain();
        void CreateDescriptorPool();

        void WaitForIdle() override;

        struct VulkanExtensionSet
        {
            std::unordered_set<std::string> instance;
            std::unordered_set<std::string> layers;
            std::unordered_set<std::string> device;
        };

        // minimal set of required extensions
        VulkanExtensionSet enabledExtensions =
        {
            // instance
            { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME },
            // layers
            { }, 
            // device
            { VK_KHR_MAINTENANCE1_EXTENSION_NAME } 
        };

        // optional extensions
        VulkanExtensionSet optionalExtensions =
        {
            // instance
            {
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME
            }, 
            // layers
            { },
            // device
            {
              VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
              VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
              VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
              VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
              VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
              VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
              VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
              VK_NV_MESH_SHADER_EXTENSION_NAME,
             } 
        };

        std::unordered_set<std::string> m_RayTracingExtensions = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
        };

        std::string m_RendererString;
        vk::Instance m_VulkanInstance;
        vk::DebugReportCallbackEXT m_DebugReportCallback;

        vk::PhysicalDevice m_VulkanPhysicalDevice;
        i32 m_GraphicsQueueFamily = -1;
        i32 m_ComputeQueueFamily = -1;
        i32 m_TransferQueueFamily = -1;
        i32 m_PresentQueueFamily = -1;

        vk::Device m_VulkanDevice;
        vk::Queue m_GraphicsQueue;
        vk::Queue m_ComputeQueue;
        vk::Queue m_TransferQueue;
        vk::Queue m_PresentQueue;

        vk::SurfaceKHR m_WindowSurface;
        vk::SurfaceFormatKHR m_SwapChainFormat;
        vk::SwapchainKHR m_SwapChain;
        vk::RenderPass m_RenderPass;
        vk::PipelineCache m_PipelineCache;
        vk::DescriptorPool m_DescriptorPool;

        bool m_SwapChainMutableFormatSupported = false;

        struct SwapChainImage
        {
            vk::Image image;
            nvrhi::TextureHandle rhiHandle;
        };

        std::vector<SwapChainImage> m_SwapChainImages;
        u32 m_SwapChainIndex = static_cast<u32>(-1);

        nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
        nvrhi::DeviceHandle m_ValidationLayer;

        std::vector<vk::Semaphore> m_AcquireSemaphores;
        std::vector<vk::Semaphore> m_PresentSemaphores;

        u32 m_AcquireSemaphoreIndex = 0;
        u32 m_PresentSemaphoreIndex = 0;

        std::queue<nvrhi::EventQueryHandle> m_FramesInFlight;
        std::vector<nvrhi::EventQueryHandle> m_QueryPool;

        bool m_BufferDeviceAddressSupported = false;

#if defined(VK_HEADER_VERSION) && (VK_HEADER_VERSION >= 301)
        typedef vk::detail::DynamicLoader VulkanDynamicLoader;
#else
        typedef vk::DynamicLoader VulkanDynamicLoader;
#endif

        std::unique_ptr<VulkanDynamicLoader> m_DynamicLoader;
    };
}
