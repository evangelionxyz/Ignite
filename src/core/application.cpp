#include "application.hpp"
#include "renderer/shader_factory.hpp"
#include <nvrhi/utils.h>

Application::Application(const ApplicationCreateInfo &createInfo)
    : m_CreateInfo(createInfo)
{
}
