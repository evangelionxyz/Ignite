#include "math.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace ignite
{
    bool Math::DecomposeTransform(const glm::mat4 &in_transform, glm::vec3 &translation, glm::quat &out_orientation, glm::vec3 &out_scale)
    {
        // From glm::decompose in matrix_decompose.inl

        using namespace glm;
        using T = float;

        mat4 LocalMatrix(in_transform);

        // Normalize the matrix.
        if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
            return false;

        // First, isolate perspective.  This is the messiest.
        if (
            epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
        {
            // Clear the perspective partition
            LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
            LocalMatrix[3][3] = static_cast<T>(1);
        }

        // Next take care of translation (easy).
        translation = vec3(LocalMatrix[3]);
        LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

        vec3 Row[3], Pdum3{};

        // Now get scale and shear.
        for (length_t i = 0; i < 3; ++i)
            for (length_t j = 0; j < 3; ++j)
                Row[i][j] = LocalMatrix[i][j];

        // Compute X scale factor and normalize first row.
        out_scale.x = length(Row[0]);
        Row[0] = detail::scale(Row[0], static_cast<T>(1));
        out_scale.y = length(Row[1]);
        Row[1] = detail::scale(Row[1], static_cast<T>(1));
        out_scale.z = length(Row[2]);
        Row[2] = detail::scale(Row[2], static_cast<T>(1));

        // At this point, the matrix (in rows[]) is orthonormal.
        // Check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.
    #if 0
        Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
        if (dot(Row[0], Pdum3) < 0)
        {
            for (length_t i = 0; i < 3; i++)
            {
                scale[i] *= static_cast<T>(-1);
                Row[i] *= static_cast<T>(-1);
            }
        }
    #endif

        out_orientation.y = asin(-Row[0][2]);
        if (cos(out_orientation.y) != 0)
        {
            out_orientation.x = atan2(Row[1][2], Row[2][2]);
            out_orientation.z = atan2(Row[0][1], Row[0][0]);
        }
        else
        {
            out_orientation.x = atan2(-Row[2][0], Row[1][1]);
            out_orientation.z = 0;
        }


        return true;
    }

    bool Math::DecomposeTransformEuler(const glm::mat4 &transform, glm::vec3 &out_translation, glm::vec3 &out_rotation, glm::vec3 &out_scale)
    {
        // From glm::decompose in matrix_decompose.inl

        using namespace glm;
        using T = float;

        mat4 LocalMatrix(transform);

        // Normalize the matrix.
        if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
            return false;

        // First, isolate perspective.  This is the messiest.
        if (
            epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
        {
            // Clear the perspective partition
            LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
            LocalMatrix[3][3] = static_cast<T>(1);
        }

        // Next take care of translation (easy).
        out_translation = vec3(LocalMatrix[3]);
        LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

        vec3 Row[3];

        // Now get scale and shear.
        for (length_t i = 0; i < 3; ++i)
            for (length_t j = 0; j < 3; ++j)
                Row[i][j] = LocalMatrix[i][j];

        // Compute X scale factor and normalize first row.
        out_scale.x = length(Row[0]);
        Row[0] = detail::scale(Row[0], static_cast<T>(1));
        out_scale.y = length(Row[1]);
        Row[1] = detail::scale(Row[1], static_cast<T>(1));
        out_scale.z = length(Row[2]);
        Row[2] = detail::scale(Row[2], static_cast<T>(1));

        // At this point, the matrix (in rows[]) is orthonormal.
        // Check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.
    #if 0
        Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
        if (dot(Row[0], Pdum3) < 0)
        {
            for (length_t i = 0; i < 3; i++)
            {
                scale[i] *= static_cast<T>(-1);
                Row[i] *= static_cast<T>(-1);
            }
        }
    #endif

        out_rotation.y = asin(-Row[0][2]);
        if (cos(out_rotation.y) != 0)
        {
            out_rotation.x = atan2(Row[1][2], Row[2][2]);
            out_rotation.z = atan2(Row[0][1], Row[0][0]);
        }
        else
        {
            out_rotation.x = atan2(-Row[2][0], Row[1][1]);
            out_rotation.z = 0;
        }

        return true;
    }

    glm::vec3 Math::Normalize(const glm::vec3 &v)
    {
        float l = glm::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (l > 0.0f)
            return { v.x / l, v.y / l, v.z / l };
        return v;
    }

    glm::vec3 Math::WorldToScreen(const glm::vec3 &world_position, const glm::mat4 &model_transform, const glm::mat4 &view_projection, const glm::vec2 &screen_size)
    {
        glm::vec4 modelPos = model_transform * glm::vec4(world_position, 1.0f);
        glm::vec4 clipPos = view_projection * modelPos;

        glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;

        glm::vec3 screenSpacePos;
        screenSpacePos.x = (ndcPos.x + 1.0f) * 0.5f * screen_size.x;
        screenSpacePos.y = (1.0f - ndcPos.y) * 0.5f * screen_size.y;
        screenSpacePos.z = ndcPos.z;
        return screenSpacePos;
    }

    glm::vec2 Math::GetNormalizedDeviceCoord(const glm::vec2 &position, const glm::vec2 &screen)
    {
        float x = (2.0f * position.x) / screen.x - 1.0f;
        float y = 1.0f - (2.0f * position.y) / screen.y;
        return { x, y };
    }

    glm::vec4 Math::GetEyeCoord(glm::vec4 clipCoords, const glm::mat4 &projectionMatrix)
    {
        glm::mat4 inverseProjection = glm::inverse(projectionMatrix);
        glm::vec4 eyeCoords = inverseProjection * clipCoords;
        return { eyeCoords.x, eyeCoords.y, -1.0f, 0.0f };
    }

    glm::vec3 Math::GetWorldPosition(const glm::vec4 &eyeCoords, const glm::mat4 &viewMatrix)
    {
        glm::vec4 worldCoords = glm::inverse(viewMatrix) * eyeCoords;
        return glm::normalize(glm::vec3(worldCoords));
    }

    glm::vec3 Math::GetRayFromScreenCoords(const glm::vec2 &coord, const glm::vec2 &screen, const glm::mat4 &projection, const glm::mat4 &view, bool isPerspective, glm::vec3 &outRayOrigin)
    {
        glm::vec2 ndc = GetNormalizedDeviceCoord(coord, screen);
        glm::vec4 hmc = glm::vec4(ndc.x, -ndc.y, -1.0f, 1.0f);

        if (isPerspective)
        {
            glm::vec4 eye = GetEyeCoord(hmc, projection);
            outRayOrigin = glm::vec3(glm::inverse(view) * glm::vec4(0, 0, 0, 1));
            return GetWorldPosition(eye, view);
        }
        else
        {
            glm::mat4 invViewProj = glm::inverse(projection * view);
            glm::vec4 worldCoords = invViewProj * hmc;
            outRayOrigin = glm::vec3(worldCoords) / worldCoords.w;
            return -glm::normalize(glm::vec3(view[2])); // Forward direction in world space
        }
    }

    bool Math::RaySphereIntersection(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const glm::vec3 &sphereCenter, float sphereRadius)
    {
        glm::vec3 oc = rayOrigin - sphereCenter;
        float a = glm::dot(rayDirection, rayDirection);
        float b = 2.0f * glm::dot(oc, rayDirection);
        float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
        float discriminant = b * b - 4 * a * c;
        return (discriminant > 0);
    }

    glm::mat4 Math::AssimpToGlmMatrix(aiMatrix4x4 mat)
    {
        glm::mat4 m;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                m[x][y] = mat[y][x];
            }
        }
        return m;
    }

    glm::vec3 Math::AssimpToGlmVec3(aiVector3D vec)
    {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    glm::quat Math::AssimpToGlmQuat(aiQuaternion quat)
    {
        glm::quat q;
        q.x = quat.x;
        q.y = quat.y;
        q.z = quat.z;
        q.w = quat.w;
        return q;
    }

#if 0
    physx::PxVec3 Math::GlmToPhysXVec3(const glm::vec3 &vec)
    {
        return physx::PxVec3{ vec.x, vec.y, vec.z };
    }

    physx::PxQuat Math::GlmToPhysXQuat(const glm::quat &quat)
    {
        return physx::PxQuat{ quat.x, quat.y, quat.z, quat.w };
    }

    glm::vec3 Math::PhysXToGlmVec3(const physx::PxVec3 &vec)
    {
        return glm::vec3{ vec.x, vec.y, vec.z };
    }

    glm::quat Math::PhysXToGlmQuat(const physx::PxQuat quat)
    {
        return glm::quat{ quat.w, quat.x, quat.y, quat.z };
    }

    physx::PxTransform Math::GlmToPhysXMatrix(const glm::mat4 &transform)
    {
        glm::vec3 position, rotation, scale;
        Math::DecomposeTransformEuler(transform, position, rotation, scale);
        return physx::PxTransform(GlmToPhysXVec3(position), GlmToPhysXQuat(rotation));
    }

    glm::vec3 Math::PhysXQuatToGlmVec3(const physx::PxQuat &quat)
    {
        glm::vec3 rotation;

        using namespace glm;
        using T = float;

        mat4 LocalMatrix(toMat4(glm::quat(quat.w, quat.x, quat.y, quat.z)));
        if (
            epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
        {
            LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
            LocalMatrix[3][3] = static_cast<T>(1);
        }

        vec3 Row[3];

        for (length_t i = 0; i < 3; ++i)
            for (length_t j = 0; j < 3; ++j)
                Row[i][j] = LocalMatrix[i][j];

        Row[0] = detail::scale(Row[0], static_cast<T>(1));
        Row[1] = detail::scale(Row[1], static_cast<T>(1));
        Row[2] = detail::scale(Row[2], static_cast<T>(1));

        rotation.y = asin(-Row[0][2]);
        if (cos(rotation.y) != 0)
        {
            rotation.x = atan2(Row[1][2], Row[2][2]);
            rotation.z = atan2(Row[0][1], Row[0][0]);
        }
        else
        {
            rotation.x = atan2(-Row[2][0], Row[1][1]);
            rotation.z = 0;
        }

        return rotation;
    }
#endif

    glm::mat4 Math::RemoveScale(const glm::mat4 &matrix)
    {
        glm::vec3 scale(
            glm::length(glm::vec3(matrix[0])),
            glm::length(glm::vec3(matrix[1])),
            glm::length(glm::vec3(matrix[2]))
        );

        glm::mat4 result = matrix;
        result[0] /= scale.x;  // normalize X axis
        result[1] /= scale.y;  // normalize Y axis
        result[2] /= scale.z;  // normalize Z axis

        return result;
    }

    float Math::CascadeSplit(i32 index, i32 cascade_count, f32 near_plane, f32 far_plane, f32 lambda)
    {
        f32 linear_split = near_plane + (far_plane - near_plane) * (index / cascade_count);
        f32 log_split = near_plane * static_cast<f32>(pow(far_plane / near_plane, index / cascade_count));
        return lambda * log_split + (1.0f - lambda) * linear_split;
    }

    void Math::ComputeCascadeMatrices(const glm::vec3 &camera_pos, const glm::mat4 &camera_view, const glm::mat4 &camera_projection, const glm::vec3 light_direction, i32 cascade_count, const std::vector<f32> &cascade_splits, std::vector<glm::mat4> &cascade_light_matrices)
    {
        glm::mat4 light_view_matrix = glm::lookAt(-light_direction * 1000.0f, glm::vec3(0.0f), glm::vec3(0, 1, 0));

        for (i32 i = 0; i < cascade_count; ++i)
        {
            glm::vec4 frustum_corners[8];
            ExtractFrustumCorners(camera_view, camera_projection, cascade_splits[i - 1], cascade_splits[i], frustum_corners);

            glm::vec3 min_bounds = glm::vec3(FLT_MAX);
            glm::vec3 max_bounds = glm::vec3(-FLT_MAX);

            for (i32 j = 0; j < 8; ++j)
            {
                glm::vec3 corner_light_space = glm::vec3(light_view_matrix * frustum_corners[j]);
                min_bounds = glm::min(min_bounds, corner_light_space);
                max_bounds = glm::min(max_bounds, corner_light_space);
            }

            glm::mat4 light_projection_matrix = glm::ortho(min_bounds.x, max_bounds.x, min_bounds.y, max_bounds.y, min_bounds.z, max_bounds.z);
            cascade_light_matrices[i] = light_projection_matrix * light_view_matrix;
        }
    }

    void Math::ExtractFrustumCorners(const glm::mat4 &view, const glm::mat4 &projection, f32 near_plane, f32 far_plane, glm::vec4 out_corners[8])
    {
        glm::mat4 inverse_view_projection = glm::inverse(projection * view);

        glm::vec4 ndc_corners[8] = {
            {-1, -1, 0, 1}, {1, -1, 0, 1}, {-1, 1, 0, 1}, {1, 1, 0, 1},
            {-1, -1, 1, 1}, {1, -1, 1, 1}, {-1, 1, 1, 1}, {1, 1, 1, 1},
        };

        for (int i = 0; i < 8; i++)
        {
            glm::vec4 world_corner = inverse_view_projection * ndc_corners[i];
            world_corner /= world_corner.w;
            out_corners[i] = world_corner;
        }
    }

    glm::vec3 Math::SnapToGrid(glm::vec3 position, f32 texel_size)
    {
        return glm::floor(position / texel_size) * texel_size;
    }

}