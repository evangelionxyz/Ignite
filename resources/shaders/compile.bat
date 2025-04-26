@echo off
ShaderMake.exe -p DXIL --binary -O3 -c "Shader.cfg" -o "dxil" --compiler "%VULKAN_SDK%\Bin\dxc.exe" --colorize 

glslc spirv-src/default_2d.vert -o spirv/default_2d_vert.spirv
glslc spirv-src/default_2d.frag -o spirv/default_2d_frag.spirv

glslc spirv-src/test_2d.vert -o spirv/test_2d_vert.spirv
glslc spirv-src/test_2d.frag -o spirv/test_2d_frag.spirv

PAUSE
