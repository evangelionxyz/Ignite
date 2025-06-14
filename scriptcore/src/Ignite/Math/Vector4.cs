namespace Ignite
{
    public struct Vector4
    {
        public float X, Y, Z, W;

        public Vector4(float scalar)
        {
            X = scalar;
            Y = scalar;
            Z = scalar;
            W = scalar;
        }

        public Vector4(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public Vector4(Vector4 vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
            W = vector.W;
        }

        public static Vector4 Zero => new Vector4(0.0f);

        public Vector4(Vector2 vector, float z, float w)
        {
            X = vector.X;
            Y = vector.Y;
            Z = z;
            W = w;
        }

        public Vector4(Vector3 vector, float w)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
            W = w;
        }

        public static Vector4 operator +(Vector4 vectorA, Vector4 vectorB)
        {
            return new Vector4(vectorA.X + vectorB.X, vectorA.Y + vectorB.Y, vectorA.Z + vectorB.Z, vectorA.W + vectorB.W);
        }

        public static Vector4 operator -(Vector4 vectorA, Vector4 vectorB)
        {
            return new Vector4(vectorA.X - vectorB.X, vectorA.Y - vectorB.Y, vectorA.Z - vectorB.Z, vectorA.W - vectorB.W);
        }

        public static Vector4 operator *(Vector4 vectorA, float scalar)
        {
            return new Vector4(vectorA.X * scalar, vectorA.Y * scalar, vectorA.Z * scalar, vectorA.W * scalar);
        }

        public static Vector4 operator /(Vector4 vectorA, float scalar)
        {
            return new Vector4(vectorA.X / scalar, vectorA.Y / scalar, vectorA.Z / scalar, vectorA.W / scalar);
        }
    }
}
