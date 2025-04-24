@echo off
ShaderMake.exe -p DXIL --binary -O3 -c "Shader.cfg" -o "dxil" --compiler "C:\SDKs\VulkanSDK\1.3.296.0\Bin\dxc.exe" --force --colorize 
ShaderMake.exe -p SPIRV --binary -O3 -c "Shader.cfg" -o "spirv" --compiler "C:\SDKs\VulkanSDK\1.3.296.0\Bin\dxc.exe" --force --colorize

glslc default_2d_vertex.vert -o spirv/default_2d_vertex.spirv
glslc default_2d_pixel.frag -o spirv/default_2d_pixel.spirv

PAUSE
