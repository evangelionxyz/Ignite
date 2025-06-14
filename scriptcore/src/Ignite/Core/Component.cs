using System;
namespace Ignite
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }

    public class Transform : Component
    {
        public Vector3 Forward
        {
            get
            {
                InternalCalls.TransformComponent_GetForward(Entity.ID, out Vector3 forward);
                return forward;
            }

            set
            {
                InternalCalls.TransformComponent_SetForward(Entity.ID, value);
            }
        }

        public Vector3 Right
        {
            get
            {
                InternalCalls.TransformComponent_GetRight(Entity.ID, out Vector3 right);
                return right;
            }

            set
            {
                InternalCalls.TransformComponent_SetRight(Entity.ID, value);
            }
        }

        public Vector3 Up
        {
            get
            {
                InternalCalls.TransformComponent_GetUp(Entity.ID, out Vector3 up);
                return up;
            }

            set
            {
                InternalCalls.TransformComponent_SetUp(Entity.ID, value);
            }
        }

        public Vector3 Translation
        {
            get
            {
                InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation);
                return translation;
            }
            set
            {
                InternalCalls.TransformComponent_SetTranslation(Entity.ID, value);
            }
        }

        public Quaternion Rotation
        {
            get
            {
                InternalCalls.TransformComponent_GetRotation(Entity.ID, out Quaternion quat);
                return quat;
            }
            set
            {
                InternalCalls.TransformComponent_SetRotation(Entity.ID, value);
            }
        }

        public Vector3 EulerAngles
        {
            get
            {
                InternalCalls.TransformComponent_GetEulerAngles(Entity.ID, out Vector3 quat);
                return quat;
            }
            set
            {
                InternalCalls.TransformComponent_SetEulerAngles(Entity.ID, value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.TransformComponent_SetScale(Entity.ID, value);
            }
        }
    }
}
