using System;

namespace Ignite
{
    public class Quaternion
    {
        public float X;
        public float Y;
        public float Z;
        public float W;

        public Quaternion(float x, float y, float z, float w)
        {
            this.X = x;
            this.Y = y;
            this.Z = z;
            this.W = w;
        }

        public Quaternion(Vector4 vec)
        {
            this.X = vec.X;
            this.Y = vec.Y;
            this.Z = vec.Z;
            this.W = vec.W;
        }

        public static Quaternion Identity => new Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        public float Magnitude => (float)Mathf.Sqrt(X * X + Y * Y + Z * Z + W * W);

        public Quaternion Normalized
        {
            get
            {
                float mag = Magnitude;
                if (mag > 0.0f)
                {
                    return new Quaternion(X / mag, Y / mag, Z / mag, W / mag);
                }
                return Identity;
            }
        }

        public static Quaternion operator *(Quaternion a, Quaternion b)
        {
            return new Quaternion(
                a.W * b.X + a.X * b.W + a.Y * b.Z - a.Z * b.Y,
                a.W * b.Y + a.Y * b.W + a.Z * b.X - a.X * b.Z,
                a.W * b.Z + a.Z * b.W + a.X * b.Y - a.Y * b.X,
                a.W * b.W - a.X * b.X - a.Y * b.Y - a.Z * b.Z
            );
        }

        public static Vector3 operator *(Quaternion rotation, Vector3 point)
        {
            float x = rotation.X * 2f;
            float y = rotation.Y * 2f;
            float z = rotation.Z * 2f;
            float xx = rotation.X * x;
            float yy = rotation.Y * y;
            float zz = rotation.Z * z;
            float xy = rotation.X * y;
            float xz = rotation.X * z;
            float yz = rotation.Y * z;
            float wx = rotation.W * x;
            float wy = rotation.W * y;
            float wz = rotation.W * z;

            Vector3 res;
            res.X = (1f - (yy + zz)) * point.X + (xy - wz) * point.Y + (xz + wy) * point.Z;
            res.Y = (xy + wz) * point.X + (1f - (xx + zz)) * point.Y + (yz - wx) * point.Z;
            res.Z = (xz - wy) * point.X + (yz + wx) * point.Y + (1f - (xx + yy)) * point.Z;
            return res;
        }

        public static Quaternion FromEuler(float pitch, float yaw, float roll)
        {
            float c1 = (float)Math.Cos(yaw / 2);
            float c2 = (float)Math.Cos(pitch / 2);
            float c3 = (float)Math.Cos(roll / 2);
            float s1 = (float)Math.Sin(yaw / 2);
            float s2 = (float)Math.Sin(pitch / 2);
            float s3 = (float)Math.Sin(roll / 2);

            return new Quaternion(
                s1 * c2 * c3 + c1 * s2 * s3,
                c1 * s2 * c3 - s1 * c2 * s3,
                c1 * c2 * s3 + s1 * s2 * c3,
                c1 * c2 * c3 - s1 * s2 * s3
            );
        }

        public static Quaternion Euler(float pitch, float yaw, float roll)
        {
            return FromEuler(pitch, yaw, roll);
        }

        public override string ToString()
        {
            return $"Quaternion({X}, {Y}, {Z}, {W})";
        }
    }
}
