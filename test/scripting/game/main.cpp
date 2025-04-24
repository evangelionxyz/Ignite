#include <iostream>
#include <Windows.h>

#include <scripting/iscript.hpp>
#include <scripting/script_component.hpp>

#include <thread>
#include <chrono>

#include <iostream>

using namespace ignite;

HMODULE LoadScriptDLL(const std::string &dllPath)
{
    HMODULE hDLL = LoadLibraryA(dllPath.c_str());
    if (!hDLL)
    {
        std::cerr << "Failed to load DLL" << dllPath << std::endl;
        return NULL;
    }

    using RegisterScriptFuc = void (*)();
    RegisterScriptFuc registerScripts = (RegisterScriptFuc)GetProcAddress(hDLL, "RegisterScripts");

    if (registerScripts)
    {
        registerScripts();
        std::cerr << "Loaded and registered scripts from: " << dllPath << std::endl;
        return hDLL;
    }

    FreeLibrary(hDLL);
    return nullptr;
};

int main()
{
    HMODULE hdll = LoadScriptDLL("GameAssembly.dll");
    if (!hdll)
    {
        std::cerr << "Invalid dll file" << std::endl;
        return 1;
    }

    std::cout << "Available scripts: " << std::endl;
    for (const auto &scriptName : IScript::GetAvailableScripts())
    {
        std::cout << "- " << scriptName << std::endl;
    }

    ScriptComponent *scriptComponent = new ScriptComponent();
    scriptComponent->AddScript("SimpleScript");

    auto last_frame = std::chrono::high_resolution_clock::now();

    while (true)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto timestep = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        last_frame = now;

        scriptComponent->Update(timestep);

        std::cout << static_cast<double>(timestep) * 0.001 << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return 0;
}