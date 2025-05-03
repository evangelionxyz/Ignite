project "ASSIMP"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    architecture "x64"

    targetdir (THIRDPARTY_OUTPUT_DIR)
    objdir (INTOUTPUT_DIR)

    files {
        "%{THIRDPARTY_DIR}/ASSIMP/include/**",
        "%{THIRDPARTY_DIR}/ASSIMP/code/Common/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/CApi/**.cpp",

        "%{THIRDPARTY_DIR}/ASSIMP/code/Pbrt/PbrtExporter.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/PostProcessing/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/Material/MaterialSystem.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/Geometry/GeometryUtils.cpp",

        --"%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/Ply/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/FBX/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/M3D/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/MD5/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/IQM/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/Collada/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/Obj/**.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/glTF/glTFCommon.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/glTF/glTFImporter.cpp",
        "%{THIRDPARTY_DIR}/ASSIMP/code/AssetLib/glTF2/glTF2Importer.cpp",

        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/adler32.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/compress.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/crc32.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/deflate.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/gzclose.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/gzlib.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/gzread.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/gzwrite.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/infback.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/inffast.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/inflate.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/inftrees.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/trees.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/uncompr.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib/zutil.c",

        "%{THIRDPARTY_DIR}/ASSIMP/contrib/unzip/ioapi.c",
        "%{THIRDPARTY_DIR}/ASSIMP/contrib/unzip/unzip.c",
	}
	
	includedirs {
		  "%{THIRDPARTY_DIR}/Assimp",
		  "%{THIRDPARTY_DIR}/ASSIMP/code",
		  "%{THIRDPARTY_DIR}/ASSIMP/include",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib/zip",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib/zlib",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib/unzip",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib/pugixml/src",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib/utf8cpp/source",
		  "%{THIRDPARTY_DIR}/ASSIMP/contrib/rapidjson/include",
      "%{THIRDPARTY_DIR}/ASSIMP/contrib/openddlparser/include",
	}

	defines {
      "RAPIDJSON_HAS_STDSTRING=1",
      -- "SWIG",
      -- "ASSIMP_BUILD_NO_OWN_ZLIB",
      -- "ASSIMP_BUILD_NO_COLLADA_IMPORTER",
      -- "ASSIMP_BUILD_NO_OBJ_IMPORTER",
      -- "ASSIMP_BUILD_NO_GLTF_IMPORTER",
      -- "ASSIMP_BUILD_NO_MD5_IMPORTER",
      -- "ASSIMP_BUILD_NO_FBX_IMPORTER",
      "ASSIMP_BUILD_NO_X_IMPORTER",
      "ASSIMP_BUILD_NO_3DS_IMPORTER",
      "ASSIMP_BUILD_NO_MD3_IMPORTER",
      "ASSIMP_BUILD_NO_MDL_IMPORTER",
      "ASSIMP_BUILD_NO_MD2_IMPORTER",
      "ASSIMP_BUILD_NO_PLY_IMPORTER",
      "ASSIMP_BUILD_NO_ASE_IMPORTER",
      "ASSIMP_BUILD_NO_AMF_IMPORTER",
      "ASSIMP_BUILD_NO_HMP_IMPORTER",
      "ASSIMP_BUILD_NO_SMD_IMPORTER",
      "ASSIMP_BUILD_NO_MDC_IMPORTER",
      "ASSIMP_BUILD_NO_STL_IMPORTER",
      "ASSIMP_BUILD_NO_LWO_IMPORTER",
      "ASSIMP_BUILD_NO_DXF_IMPORTER",
      "ASSIMP_BUILD_NO_NFF_IMPORTER",
      "ASSIMP_BUILD_NO_RAW_IMPORTER",
      "ASSIMP_BUILD_NO_OFF_IMPORTER",
      "ASSIMP_BUILD_NO_AC_IMPORTER",
      "ASSIMP_BUILD_NO_BVH_IMPORTER",
      "ASSIMP_BUILD_NO_IRRMESH_IMPORTER",
      "ASSIMP_BUILD_NO_IRR_IMPORTER",
      "ASSIMP_BUILD_NO_Q3D_IMPORTER",
      "ASSIMP_BUILD_NO_B3D_IMPORTER",
      "ASSIMP_BUILD_NO_TERRAGEN_IMPORTER",
      "ASSIMP_BUILD_NO_CSM_IMPORTER",
      "ASSIMP_BUILD_NO_3D_IMPORTER",
      "ASSIMP_BUILD_NO_LWS_IMPORTER",
      "ASSIMP_BUILD_NO_OGRE_IMPORTER",
      "ASSIMP_BUILD_NO_OPENGEX_IMPORTER",
      "ASSIMP_BUILD_NO_MS3D_IMPORTER",
      "ASSIMP_BUILD_NO_COB_IMPORTER",
      "ASSIMP_BUILD_NO_BLEND_IMPORTER",
      "ASSIMP_BUILD_NO_Q3BSP_IMPORTER",
      "ASSIMP_BUILD_NO_NDO_IMPORTER",
      "ASSIMP_BUILD_NO_IFC_IMPORTER",
      "ASSIMP_BUILD_NO_XGL_IMPORTER",
      "ASSIMP_BUILD_NO_ASSBIN_IMPORTER",
      "ASSIMP_BUILD_NO_C4D_IMPORTER",
      "ASSIMP_BUILD_NO_3MF_IMPORTER",
      "ASSIMP_BUILD_NO_X3D_IMPORTER",
      "ASSIMP_BUILD_NO_MMD_IMPORTER",
      "ASSIMP_BUILD_NO_STEP_EXPORTER",
      "ASSIMP_BUILD_NO_SIB_IMPORTER",     
      "ASSIMP_BUILD_NO_USD_IMPORTER",
      "ASSIMP_BUILD_NO_PBRT_IMPORTER",
  }

    filter "system:linux"
        pic "On"

    filter "system:windows"
        systemversion "latest"
        defines {
            "_CRT_SECURE_NO_WARNINGS",
        }
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
    filter "configurations:Release"
        runtime "Release"
        optimize "on"
    filter "configurations:Dist"
        runtime "Release"
        optimize "on"