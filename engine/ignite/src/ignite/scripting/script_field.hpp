#pragma once

#include <string>

extern "C" 
{
    typedef struct _MonoClassField MonoClassField;
}

namespace ignite
{
    enum class ScriptFieldType
    {
        Invalid = 0,
        Entity,
        Float, 
        Double,
        Bool, 
        Char,
        Byte, 
        Short, 
        Int, 
        Long,
        UByte, 
        UShort, 
        UInt, 
        ULong,
        Vector2, 
        Vector3, 
        Vector4
    };

    struct ScriptField
    {
        ScriptFieldType Type = ScriptFieldType::Invalid;
        std::string Name;

        MonoClassField *ClassField = nullptr;
    };
}