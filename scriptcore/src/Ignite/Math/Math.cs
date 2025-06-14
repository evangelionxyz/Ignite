using System;

namespace Ignite
{
    public static class Mathf
    {
        public const float PI = 3.14159265358979323846f;
        public const float Deg2Rad = PI / 180f;
        public const float Rad2Deg = 180f / PI;
        public const float Epsilon = 1e-5f;

        public static float Sin(float x) => (float)Math.Sin(x);
        public static float Cos(float x) => (float)Math.Cos(x);
        public static float Tan(float x) => (float)Math.Tan(x);

        public static float Asin(float x) => (float)Math.Asin(x);
        public static float Acos(float x) => (float)Math.Acos(x);
        public static float Atan(float x) => (float)Math.Atan(x);
        public static float Atan2(float y, float x) => (float)Math.Atan2(y, x);

        public static float Sqrt(float x) => (float)Math.Sqrt(x);
        public static float Pow(float x, float y) => (float)Math.Pow(x, y);

        public static float Abs(float x) => Math.Abs(x);

        public static float Min(float a, float b) => (a < b) ? a : b;
        public static float Max(float a, float b) => (a > b) ? a : b;

        public static float Clamp(float value, float min, float max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        public static float Lerp(float a, float b, float t)
        {
            return a + (b - a) * Clamp(t, 0f, 1f);
        }

        public static bool Approximately(float a, float b)
        {
            return Abs(b - a) < Max(1e-6f * Max(Abs(a), Abs(b)), Epsilon * 8);
        }

        public static float InverseLerp(float a, float b, float value)
        {
            if (a != b)
                return Clamp((value - a) / (b - a), 0f, 1f);
            return 0f;
        }

        public static float DeltaAngle(float current, float target)
        {
            float delta = target - current;
            while (delta > 180) delta -= 360;
            while (delta < -180) delta += 360;
            return delta;
        }

        public static float SmoothStep(float edge0, float edge1, float x)
        {
            x = Clamp((x - edge0) / (edge1 - edge0), 0f, 1f);
            return x * x * (3 - 2 * x);
        }

        public static int FloorToInt(float f) => (int)Math.Floor(f);
        public static int CeilToInt(float f) => (int)Math.Ceiling(f);
        public static int RoundToInt(float f) => (int)Math.Round(f);

    }
}
