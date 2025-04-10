#pragma once

class DeviceManager;

class Renderer
{
public:
    static void Init(DeviceManager *deviceManager);
    static void Shutdown();
};
