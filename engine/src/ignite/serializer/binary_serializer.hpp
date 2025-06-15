#pragma once

#include <vector>
#include <cinttypes>

#include "ignite/animation/skeleton.hpp"

namespace ignite
{
    struct FileHeader
    {
        char engineVersion[4]; // 4 B OFFSET 0
        char magic[4];         // 4 B OFFSET 4 - identifier (e.g SKEL for skeleton)
        char fileFormat[3];    // 3 B OFFSET 8 - major.minor.revision
        uint8_t compression;   // 1 B OFFSET 10 - compression flag (0 = none, 1 = zlib)
        // ^ 12 bytes

        uint64_t payloadSize;   // 8 B OFFSET 12 - uncompressed size, Fast sanity check, file length
        char userDetails[36];   // 36 B OFFSET 20 - e.g artist name, tool version, date
    };

    struct DiskJoint
    {
        uint32_t nameOffset;          // into the string table
        int32_t  id;
        int32_t  parentId;
        float    inverseBindPose[16]; // column‑major 4×4
        float    localTransform[16]; // column‑major 4×4
    };

    class BinarySerializer
    {
    public:
        template<typename T>
        static void AppendRaw(std::vector<std::byte> &out, const T &value)
        {
            const std::byte *raw = reinterpret_cast<const std::byte *>(&value);
            out.insert(out.end(), raw, raw + sizeof(T));
        }

        static std::vector<std::byte> SerializeSkeleton(const Skeleton &skeleton)
        {
            std::vector<std::byte> buffer;

            // write joint count
            uint32_t jountCount = static_cast<uint32_t>(skeleton.joints.size());
            AppendRaw(buffer, jountCount);

            // build the string table and map: joint name -> offset in string table
            std::string stringTable;
            std::unordered_map<std::string, uint32_t> nameOffsets;

            for (const Joint &joint : skeleton.joints)
            {
                if (!nameOffsets.contains(joint.name))
                {
                    uint32_t offset = static_cast<uint32_t>(stringTable.size());
                    nameOffsets[joint.name] = offset;
                    stringTable += joint.name;
                    stringTable += '\0'; // Null-terminate
                }
            }

            for (const Joint &joint : skeleton.joints)
            {
                DiskJoint dj{};
                dj.nameOffset = nameOffsets[joint.name];
                dj.id = joint.id;
                dj.parentId = joint.parentJointId;

                // store inverse bindpose in column-major order
                std::memcpy(dj.inverseBindPose, &joint.inverseBindPose[0].x, sizeof(float) * 16);
                std::memcpy(dj.localTransform, &joint.localTransform[0].x, sizeof(float) * 16);

                AppendRaw(buffer, dj);
            }

            // write string table size and data
            uint32_t stringSize = static_cast<uint32_t>(stringTable.size());
            AppendRaw(buffer, stringSize);

            buffer.insert(
                buffer.end(),
                reinterpret_cast<const std::byte *>(stringTable.data()),
                reinterpret_cast<const std::byte *>(stringTable.data() + stringSize)
            );

            return buffer;
        }

        static Skeleton DeserializeSkeleton(const std::filesystem::path &filepath)
        {
            Skeleton skeleton;

            std::ifstream inFile(filepath, std::ios::binary);

            if (!inFile)
            {
                throw std::runtime_error("Cannot open skeleton file " + filepath.string());
            }

            // inFile.seekg(0, std::ios::end);
            // size_t fileSize = inFile.tellg();
            // inFile.seekg(0, std::ios::beg);

            // read joint count
            uint32_t jointCount = 0;
            inFile.read(reinterpret_cast<char *>(&jointCount), sizeof(uint32_t));

            // read disk joint array
            std::vector<DiskJoint> diskJoints(jointCount);
            inFile.read(reinterpret_cast<char *>(diskJoints.data()), sizeof(DiskJoint) * jointCount);

            // read string table
            uint32_t stringTableSize = 0;
            inFile.read(reinterpret_cast<char *>(&stringTableSize), sizeof(stringTableSize));

            std::vector<char> stringTable(stringTableSize); // owns the bytes
            inFile.read(stringTable.data(), stringTableSize);


            skeleton.joints.reserve(jointCount);

            for (const DiskJoint &dj : diskJoints)
            {
                if (dj.nameOffset >= stringTableSize)
                {
                    throw std::runtime_error("Corrupt file: nameOffset out of range");
                }

                const char *namePtr = stringTable.data() + dj.nameOffset;
                std::string jointName = std::string(namePtr);

                Joint joint{};
                joint.name = jointName;
                joint.id = dj.id;
                joint.parentJointId = dj.parentId;
                joint.globalTransform = glm::mat4(1.0f);

                std::memcpy(&joint.inverseBindPose[0].x, dj.inverseBindPose, sizeof(float) * 16);
                std::memcpy(&joint.localTransform[0].x, dj.localTransform, sizeof(float) * 16);

                skeleton.joints.push_back(std::move(joint));

                skeleton.nameToJointMap[jointName] = joint.id;
            }

            return skeleton;
        }
    };
}