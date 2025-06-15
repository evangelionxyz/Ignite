#pragma once

#include "ignite/core/types.hpp"

#include <filesystem>
#include <functional>

namespace ignite::vfs
{
    namespace status
    {
        constexpr int OK = 0;
        constexpr int Failed = -1;
        constexpr int PathNotFound = -2;
        constexpr int NotImplemented = -3;
    }

    using enumerate_callback_t = const std::function<void(std::string_view)> &;

    class IBlob
    {
    public:
        virtual ~IBlob() = default;
        [[nodiscard]] virtual const void *Data() const = 0;
        [[nodiscard]] virtual size_t Size() const = 0;

        static bool IsEmpty(IBlob const *blob)
        {
            return blob == nullptr || blob->Data() == nullptr || blob->Size() == 0;
        }
    };

    class Blob : public IBlob
    {
    public:
        Blob() = default;
        Blob(void *data, size_t size);
        ~Blob() override;
        [[nodiscard]] const void *Data() const override;
        [[nodiscard]] size_t Size() const override;
    private:
        void *m_Data = nullptr;
        size_t m_Size = 0;
    };

    class IFileSystem
    {
    public:
        virtual ~IFileSystem() = default;

        virtual bool DirectoryExists(const std::filesystem::path &path) = 0;
        virtual bool FileExists(const std::filesystem::path &path) = 0;

        virtual Ref<IBlob> ReadFile(const std::filesystem::path &path) = 0;
        virtual bool WriteFile(const std::filesystem::path &path, const void *data, const size_t size) = 0;

        virtual int EnumerateFiles(const std::filesystem::path &path, const std::vector<std::string> &extensions, enumerate_callback_t callback, bool allowDuplicates = false) = 0;
        virtual int EnumerateDirectories(const std::filesystem::path &path, enumerate_callback_t callback, bool allowDuplicates) = 0;
    };

    class NativeFileSystem : public IFileSystem
    {
    public:
        virtual bool DirectoryExists(const std::filesystem::path &path) override;
        virtual bool FileExists(const std::filesystem::path &path) override;

        virtual Ref<IBlob> ReadFile(const std::filesystem::path &path) override;
        virtual bool WriteFile(const std::filesystem::path &path, const void *data, const size_t size) override;

        virtual int EnumerateFiles(const std::filesystem::path &path, const std::vector<std::string> &extensions, enumerate_callback_t callback, bool allowDuplicates = false) override;
        virtual int EnumerateDirectories(const std::filesystem::path &path, enumerate_callback_t callback, bool allowDuplicates) override;
    };

    class RelativeFileSystem : public IFileSystem
    {
    public:
        RelativeFileSystem(Ref<IFileSystem> fs, const std::filesystem::path &basePath);
        [[nodiscard]] std::filesystem::path const &GetBasePath() const { return m_BasePath; }

        virtual bool DirectoryExists(const std::filesystem::path &path) override;
        virtual bool FileExists(const std::filesystem::path &path) override;

        virtual Ref<IBlob> ReadFile(const std::filesystem::path &path) override;
        virtual bool WriteFile(const std::filesystem::path &path, const void *data, const size_t size) override;

        virtual int EnumerateFiles(const std::filesystem::path &path, const std::vector<std::string> &extensions, enumerate_callback_t callback, bool allowDuplicates = false) override;
        virtual int EnumerateDirectories(const std::filesystem::path &path, enumerate_callback_t callback, bool allowDuplicates) override;

    private:
        Ref<IFileSystem> m_UnderlyingFS;
        std::filesystem::path m_BasePath;
    };

    class RootFileSystem : public IFileSystem
    {
    public:
        void Mount(const std::filesystem::path &path, Ref<IFileSystem> fs);
        void Mount(const std::filesystem::path &path, const std::filesystem::path &nativePath);
        bool Unmount(const std::filesystem::path &path);

        virtual bool DirectoryExists(const std::filesystem::path &path) override;
        virtual bool FileExists(const std::filesystem::path &path) override;

        virtual Ref<IBlob> ReadFile(const std::filesystem::path &path) override;
        virtual bool WriteFile(const std::filesystem::path &path, const void *data, const size_t size) override;

        virtual int EnumerateFiles(const std::filesystem::path &path, const std::vector<std::string> &extensions, enumerate_callback_t callback, bool allowDuplicates = false) override;
        virtual int EnumerateDirectories(const std::filesystem::path &path, enumerate_callback_t callback, bool allowDuplicates) override;
    private:
        std::vector<std::pair<std::string, Ref<IFileSystem>>> m_MountPoints;
        bool FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS);
    };

    std::string GetFileSearchRegex(const std::filesystem::path &path, const std::vector<std::string> &extensions);
}