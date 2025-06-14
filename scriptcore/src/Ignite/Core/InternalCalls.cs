using System;
using System.Runtime.CompilerServices;

namespace Ignite
{
    public static class InternalCalls
    {
        // Entity Method
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_AddComponent(ulong entityID, Type componentType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_FindEntityByName(string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_Instantiate(ulong entityID, Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_Destroy(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_SetVisibility(ulong entityID, bool value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_GetVisibility(ulong entityID, out bool result);

        // Script Instance
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static object GetScriptInstance(ulong entityID);

        // Transform
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetForward(ulong entityID, out Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetForward(ulong entityID, Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetRight(ulong entityID, out Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetRight(ulong entityID, Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetUp(ulong entityID, out Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetUp(ulong entityID, Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetTranslation(ulong entityID, Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetRotation(ulong entityID, out Quaternion result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetRotation(ulong entityID, Quaternion value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetEulerAngles(ulong entityID, out Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetEulerAngles(ulong entityID, Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetScale(ulong entityID, out Vector3 result);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetScale(ulong entityID, Vector3 value);
    }
}
