@echo off
ShaderMake.exe -p DXIL --binary -O3 -c "Shader.cfg" -o "dxil" --compiler "C:\VulkanSDK\1.3.296.0\Bin\dxc.exe"
