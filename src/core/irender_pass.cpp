#include "irender_pass.hpp"

#include "device_manager.hpp"

DeviceManager *IRenderPass::GetDeviceManager() const 
{ 
    return m_DeviceManager; 
}

nvrhi::IDevice *IRenderPass::GetDevice() const 
{ 
    return m_DeviceManager->GetDevice();
}

u32 IRenderPass::GetFrameIndex() const 
{ 
    return m_DeviceManager->GetFrameIndex(); 
}