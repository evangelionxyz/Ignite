project "GLFW"
language "C"
kind "StaticLib"
staticruntime "off"
architecture "x64"

targetdir (THIRDPARTY_OUTPUT_DIR)
objdir (INTOUTPUT_DIR)

files {
    "GLFW/src/context.c",
    "GLFW/src/init.c",
    "GLFW/src/input.c",
    "GLFW/src/monitor.c",
    "GLFW/src/null_init.c",
    "GLFW/src/null_joystick.c",
    "GLFW/src/null_monitor.c",
    "GLFW/src/null_window.c",
    "GLFW/src/platform.c",
    "GLFW/src/window.c",
    "GLFW/src/vulkan.c",
    "GLFW/src/osmesa_context.c",
    "GLFW/src/wgl_context.c",
    "GLFW/src/egl_context.c",
}

--linux
filter "system:linux"
pic "on"
files {
    "GLFW/src/posix_time.c",
    "GLFW/src/posix_thread.c",
    "GLFW/src/posix_poll.c",
    "GLFW/src/posix_module.c",
    "GLFW/src/linux_joystick.c",
    "GLFW/src/x11_init.c",
    "GLFW/src/x11_monitor.c",
    "GLFW/src/x11_window.c",
    "GLFW/src/xkb_unicode.c",
}
defines {
    "_GLFW_X11"
}

--windows
filter "system:windows"
systemversion "latest"
files {
    "GLFW/src/win32_init.c",
    "GLFW/src/win32_joystick.c",
    "GLFW/src/win32_monitor.c",
    "GLFW/src/win32_module.c",
    "GLFW/src/win32_time.c",
    "GLFW/src/win32_thread.c",
    "GLFW/src/win32_window.c",
}
defines {
    "_GLFW_WIN32",
    "_CRT_SECURE_NO_WARNINGS"
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
