#pragma once

#include "ignite/core/types.hpp"
#include "ignite/math/math.hpp"

namespace ignite {

    template<typename T>
    struct KeyFrame
    {
        T Value;
        f32 Timestamp;
    };

    template<typename T>
    using KeyFrames = std::vector<KeyFrame<T>>;

    struct TransformKeyFrameBase
    {
    protected:
        f32 GetScaleFactor(f32 last_time_stamp, f32 next_time_stamp, f32 time)
        {
            f32 midWayLength = time - last_time_stamp;
            f32 framesDiff = next_time_stamp - last_time_stamp;
            f32 scale_factor = midWayLength / framesDiff;
            return scale_factor;
        }
    };

    struct Vec3Key : TransformKeyFrameBase
    {
        KeyFrames<glm::vec3> frames;

        void AddFrame(const KeyFrame<glm::vec3> &key_frame)
        {
            frames.push_back(key_frame);
        }

        i32 GetIndex(f32 anim_time)
        {
            for (i32 i = 0; i < frames.size() - 1; ++i)
            {
                if (anim_time < frames[i + 1].Timestamp)
                    return i;
            }

            return 0;
        }

        glm::vec3 InterpolateTranslation(f32 time)
        {
            if (frames.size() == 1)
                return frames[0].Value;

            i32 p0Index = GetIndex(time);
            i32 p1Index = p0Index + 1;
            f32 scale_factor = GetScaleFactor(frames[p0Index].Timestamp, frames[p1Index].Timestamp, time);
            return glm::mix(frames[p0Index].Value, frames[p1Index].Value, scale_factor);
        }

        glm::vec3 InterpolateScaling(f32 time)
        {
            if (frames.size() == 1)
                return frames[0].Value;
            i32 p0Index = GetIndex(time);
            i32 p1Index = p0Index + 1;
            f32 scale_factor = GetScaleFactor(frames[p0Index].Timestamp, frames[p1Index].Timestamp, time);
            return glm::mix(frames[p0Index].Value, frames[p1Index].Value, scale_factor);
        }

        glm::mat4 InterpolateTranslateMatrix(f32 time)
        {
            return glm::translate(glm::mat4(1.0f), InterpolateTranslation(time));
        }

        glm::mat4 InterpolateScaleMatrix(f32 time)
        {
            return glm::scale(glm::mat4(1.0f), InterpolateScaling(time));
        }
    };

    struct QuatKey : public TransformKeyFrameBase
    {
        KeyFrames<glm::quat> frames;

        void AddFrame(const KeyFrame<glm::quat> &keyFrame)
        {
            frames.push_back(keyFrame);
        }

        i32 GetIndex(f32 time)
        {
            for (i32 i = 0; i < frames.size() - 1; ++i)
                if (time < frames[i + 1].Timestamp)
                    return i;
            return 0;
        }

        glm::quat InterpolateRotation(f32 time)
        {
            if (frames.size() == 1)
                return glm::normalize(frames[0].Value);

            i32 p0Index = GetIndex(time);
            i32 p1Index = p0Index + 1;
            f32 scale_factor = GetScaleFactor(frames[p0Index].Timestamp, frames[p1Index].Timestamp, time);
            glm::quat final_rotation = glm::slerp(frames[p0Index].Value, frames[p1Index].Value, scale_factor);
            return glm::normalize(final_rotation);
        }

        glm::mat4 InterpolateRotationMatrix(f32 time)
        {
            return glm::toMat4(InterpolateRotation(time));
        }
    };
}
