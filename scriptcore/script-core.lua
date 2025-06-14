-- Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine
project "ScriptCore"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.8"
    
    targetdir (OUTPUT_DIR)
    objdir (INTOUTPUT_DIR)

    files {
        "src/**.cs",
        "properties/**.cs",
    }

    filter "configurations:Debug"
        optimize "On"
        symbols "Default"

    filter "configurations:Release"
        optimize "Full"
        symbols "Default"

    filter "configurations:Dist"
        optimize "Full"
        symbols "Off"

