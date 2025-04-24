project "IgniteEngine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"

    location "%{wks.location}/engine"

    outputdir "%{OUTPUT_DIR}"
    objdir "%{INTOUTPUT_DIR}"

    files {
        "src/ignite/**.cpp",
        "src/ignite/**.hpp",
        "src/ignite/**.h",
    }

    includedirs {
        
    }