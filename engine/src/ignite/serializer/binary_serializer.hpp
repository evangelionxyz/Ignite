#pragma once

#include <vector>
#include <cinttypes>

#include "ignite/animation/keyframes.hpp"
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

        static std::vector<std::byte> SerializeAnimation(const SkeletalAnimation &anim)
        {
            std::vector<std::byte> buffer;

            // write animation duration and ticks per second
            AppendRaw(buffer, anim.duration);
            AppendRaw(buffer, anim.ticksPerSeconds);

            // write name size and name
            std::string nameCopy = anim.name;
            nameCopy += '\0';
            uint32_t nameSize = static_cast<uint32_t>(nameCopy.size());
            AppendRaw(buffer, nameSize);

            buffer.insert(buffer.end(),
                reinterpret_cast<const std::byte *>(nameCopy.data()),
                reinterpret_cast<const std::byte *>(nameCopy.data() + nameSize)
            );

            // write channels
            uint32_t channelCount = static_cast<uint32_t>(anim.channels.size());
            AppendRaw(buffer, channelCount);

            for (const auto &[channelName, channel] : anim.channels)
            {
                // write uint32_t name size
                nameCopy = channelName;
                nameCopy += '\0';
                uint32_t channelNameSize = static_cast<uint32_t>(nameCopy.size());
                AppendRaw(buffer, channelNameSize);

                buffer.insert(buffer.end(),
                    reinterpret_cast<const std::byte *>(nameCopy.data()),
                    reinterpret_cast<const std::byte *>(nameCopy.data() + channelNameSize));

                uint32_t translationFrameCount = static_cast<uint32_t>(channel.translationKeys.frames.size());
                uint32_t rotationFrameCount = static_cast<uint32_t>(channel.rotationKeys.frames.size());
                uint32_t scaleFrameCountFrameCount = static_cast<uint32_t>(channel.scaleKeys.frames.size());

                // write total frame data size for validation
                uint32_t framesDataSize = 
                      translationFrameCount * sizeof(KeyFrame<glm::vec3>)
                    + rotationFrameCount * sizeof(KeyFrame<glm::quat>)
                    + scaleFrameCountFrameCount * sizeof(KeyFrame<glm::vec3>);

                AppendRaw(buffer, framesDataSize);

                // Write Translation frames
                AppendRaw(buffer, translationFrameCount);

                for (const auto &frame : channel.translationKeys.frames)
                {
                    AppendRaw(buffer, frame.Value);
                    AppendRaw(buffer, frame.Timestamp);
                }

                // Write Rotation frames
                AppendRaw(buffer, rotationFrameCount);

                for (const auto &frame : channel.rotationKeys.frames)
                {
                    AppendRaw(buffer, frame.Value);
                    AppendRaw(buffer, frame.Timestamp);
                }

                // Write Scale frames
                AppendRaw(buffer, scaleFrameCountFrameCount);

                for (const auto &frame : channel.scaleKeys.frames)
                {
                    AppendRaw(buffer, frame.Value);
                    AppendRaw(buffer, frame.Timestamp);
                }

            }

            return buffer;
        }

        static SkeletalAnimation DeserializeAnimation(const std::filesystem::path &filepath)
        {
            SkeletalAnimation anim;

            std::ifstream inFile(filepath, std::ios::binary);

            if (!inFile)
            {
                throw std::runtime_error("Cannot open animation file " + filepath.string());
            }

            // read animation duration and ticks per second
            inFile.read(reinterpret_cast<char *>(&anim.duration), sizeof(anim.duration));
            inFile.read(reinterpret_cast<char *>(&anim.ticksPerSeconds), sizeof(anim.ticksPerSeconds));

            // read name size and name
            uint32_t nameSize = 0;
            inFile.read(reinterpret_cast<char *>(&nameSize), sizeof(nameSize));
            std::vector<char> stringBytes(nameSize); // owns the bytes
            inFile.read(stringBytes.data(), nameSize);
            anim.name = std::string(stringBytes.data());

            // read channel count
            uint32_t channelCount = 0;
            inFile.read(reinterpret_cast<char *>(&channelCount), sizeof(channelCount));

            anim.channels.reserve(channelCount);

            for (uint32_t channelIdx = 0; channelIdx < channelCount; ++channelIdx)
            {
                // read channel name
                inFile.read(reinterpret_cast<char *>(&nameSize), sizeof(nameSize));
                stringBytes.resize(nameSize);
                inFile.read(stringBytes.data(), nameSize);
                std::string channelName = std::string(stringBytes.data());

                AnimationChannel channel{};


                // read total frame data in bytes (translation + rotation + scale) for validation
                uint32_t expectedTotalSize = 0;
                inFile.read(reinterpret_cast<char *>(&expectedTotalSize), sizeof(expectedTotalSize));
                uint32_t totalChannelByteSize = 0;

                // process translation keys
                uint32_t translationFrameCount = 0;
                inFile.read(reinterpret_cast<char *>(&translationFrameCount), sizeof(translationFrameCount));

                channel.translationKeys.frames.reserve(translationFrameCount);
                for (uint32_t frameIdx = 0; frameIdx < translationFrameCount; ++frameIdx)
                {
                    KeyFrame<glm::vec3> frame{};

                    inFile.read(reinterpret_cast<char *>(&frame.Value.x), sizeof(frame.Value));
                    inFile.read(reinterpret_cast<char *>(&frame.Timestamp), sizeof(frame.Timestamp));
                    totalChannelByteSize += sizeof(frame);

                    channel.translationKeys.frames.push_back(frame);
                }

                // process rotation keys
                uint32_t rotationFrameCount = 0;
                inFile.read(reinterpret_cast<char *>(&rotationFrameCount), sizeof(rotationFrameCount));

                channel.rotationKeys.frames.reserve(rotationFrameCount);
                for (uint32_t frameIdx = 0; frameIdx < rotationFrameCount; ++frameIdx)
                {
                    KeyFrame<glm::quat> frame{};

                    inFile.read(reinterpret_cast<char *>(&frame.Value.x), sizeof(frame.Value));
                    inFile.read(reinterpret_cast<char *>(&frame.Timestamp), sizeof(frame.Timestamp));
                    totalChannelByteSize += sizeof(frame);

                    channel.rotationKeys.frames.push_back(frame);
                }

                // process rotation keys
                uint32_t scaleFrameCount = 0;
                inFile.read(reinterpret_cast<char *>(&scaleFrameCount), sizeof(scaleFrameCount));

                channel.scaleKeys.frames.reserve(scaleFrameCount);
                for (uint32_t frameIdx = 0; frameIdx < scaleFrameCount; ++frameIdx)
                {
                    KeyFrame<glm::vec3> frame{};

                    inFile.read(reinterpret_cast<char *>(&frame.Value.x), sizeof(frame.Value));
                    inFile.read(reinterpret_cast<char *>(&frame.Timestamp), sizeof(frame.Timestamp));
                    totalChannelByteSize += sizeof(frame);

                    channel.scaleKeys.frames.push_back(frame);
                }

                LOG_ASSERT(expectedTotalSize == totalChannelByteSize,
                    "Corrupt animation data expected channel size {}, got {}",
                    expectedTotalSize, totalChannelByteSize);

                anim.channels[channelName] = channel;
            }

            return anim;
        }

        static std::vector<std::byte> SerializeSkeleton(const Skeleton &skeleton)
        {
            std::vector<std::byte> buffer;

            // write joint count
            uint32_t jointCount = static_cast<uint32_t>(skeleton.joints.size());
            AppendRaw(buffer, jointCount);

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