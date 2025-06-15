project "ZLIB"
kind "StaticLib"
language "C"
architecture "x64"
staticruntime "off"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "ZLIB/**.c"
}

includedirs {
    "ZLIB"
}

filter "configurations:Debug"
  runtime "Debug"
  symbols "on"

filter "configurations:Release"
  runtime "Release"
  symbols "on"
  optimize "on"

filter "configurations:Dist"
  runtime "Release"
  symbols "off"
  optimize "on"