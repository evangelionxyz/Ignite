#include "vfs.hpp"

#include "core/logger.hpp"
#include "core/string_utils.hpp"

#include <fstream>
#include <cassert>
#include <algorithm>
#include <utility>
#include <sstream>

#ifdef PLATFORM_WINDOWS
#include <Shlwapi.h>
#endif

namespace vfs
{
    Blob::Blob(void* data, size_t size)
    : m_Data(data)
    , m_Size(size)
{

}

const void* Blob::Data() const
{
    return m_Data;
}

size_t Blob::Size() const
{
    return m_Size;
}

Blob::~Blob()
{
    if (m_Data)
    {
        free(m_Data);
        m_Data = nullptr;
    }
    m_Size = 0;
}

bool NativeFileSystem::DirectoryExists(const std::filesystem::path &path)
{
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

bool NativeFileSystem::FileExists(const std::filesystem::path &path)
{
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

Ref<IBlob> NativeFileSystem::ReadFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    u64 size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > static_cast<u64>(std::numeric_limits<size_t>::max()))
    {
        LOG_ASSERT(false, "File too larege");
        return nullptr;
    }

    char *data = static_cast<char *>(malloc(size));
    if (data == nullptr)
    {
        LOG_ASSERT(false, "Out of memory");
        return nullptr;
    }

    file.read(data, size);

    if (!file.good())
    {
        free(data);
        LOG_ASSERT(false, "File reading error");
        return nullptr;
    }

    return CreateRef<Blob>(data, size);
}

bool NativeFileSystem::WriteFile(const std::filesystem::path &path, const void *data, size_t size)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    if (size > 0)
    {
        file.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
    }

    if (!file.good())
    {
        LOG_ASSERT(false, "File writing error");
        return false;
    }
    return true;
}

static int EnumerateNativeFiles(const char *pattern, bool directories, enumerate_callback_t callback)
{
#ifdef PLATFORM_WINDOWS
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return 0;
        return status::Failed;
    }
    int numEntries = 0;
    do
    {
        bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bool isDot = strcmp(findData.cFileName, ".") == 0;
        bool isDotDot = strcmp(findData.cFileName, "..") == 0;

        if ((isDirectory == directories) && !isDot && !isDotDot)
        {
            callback(findData.cFileName);
            ++numEntries;
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);

    return numEntries;

#else // WIN32

    glob64_t glob_matches;
    int globResult = glob64(pattern, 0 /*flags*/, nullptr /*errfunc*/, &glob_matches);

    if (globResult == 0)
    {
        int numEntries = 0;

        for (int i=0; i<glob_matches.gl_pathc; ++i)
        {
            const char* globentry = (glob_matches.gl_pathv)[i];
            std::error_code ec, ec2;
            std::filesystem::directory_entry entry(globentry, ec);
            if (!ec)
            {
                if (directories == entry.is_directory(ec2) && !ec2)
                {
                    callback(entry.path().filename().native());
                    ++numEntries;
                }
            }
        }
        globfree64(&glob_matches);

        return numEntries;
    }

    if (globResult == GLOB_NOMATCH)
        return 0;

    return status::Failed;
#endif // WIN32
}

int NativeFileSystem::EnumerateFiles(const std::filesystem::path &path, const std::vector<std::string> &extensions, enumerate_callback_t callback, bool allowDuplicates)
{
    (void)allowDuplicates;

    if (extensions.empty())
    {
        std::string pattern = (path / "*").generic_string();
        return EnumerateNativeFiles(pattern.c_str(), false, callback);
    }

    int numEntries = 0;
    for (const auto &ext : extensions)
    {
        std::string pattern = (path / ("*" + ext)).generic_string();
        int result = EnumerateNativeFiles(pattern.c_str(), false, callback);

        if (result < 0)
            return result;

        numEntries += result;
    }

    return numEntries;
}

int NativeFileSystem::EnumerateDirectories(const std::filesystem::path &path, enumerate_callback_t callback, bool allowDuplicates)
{
    (void)allowDuplicates;

    std::string pattern = (path / "*").generic_string();
    return EnumerateNativeFiles(pattern.c_str(), true, callback);
}

RelativeFileSystem::RelativeFileSystem(Ref<IFileSystem> fs, const std::filesystem::path& basePath)
    : m_UnderlyingFS(std::move(fs))
    , m_BasePath(basePath.lexically_normal())
{
}

bool RelativeFileSystem::DirectoryExists(const std::filesystem::path& name)
{
	return m_UnderlyingFS->DirectoryExists(m_BasePath / name.relative_path());
}

bool RelativeFileSystem::FileExists(const std::filesystem::path& name)
{
    return m_UnderlyingFS->FileExists(m_BasePath / name.relative_path());
}

Ref<IBlob> RelativeFileSystem::ReadFile(const std::filesystem::path& name)
{
    return m_UnderlyingFS->ReadFile(m_BasePath / name.relative_path());
}

bool RelativeFileSystem::WriteFile(const std::filesystem::path& name, const void* data, size_t size)
{
    return m_UnderlyingFS->WriteFile(m_BasePath / name.relative_path(), data, size);
}

int RelativeFileSystem::EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, enumerate_callback_t callback, bool allowDuplicates)
{
    return m_UnderlyingFS->EnumerateFiles(m_BasePath / path.relative_path(), extensions, callback, allowDuplicates);
}

int RelativeFileSystem::EnumerateDirectories(const std::filesystem::path& path, enumerate_callback_t callback, bool allowDuplicates)
{
    return m_UnderlyingFS->EnumerateDirectories(m_BasePath / path.relative_path(), callback, allowDuplicates);
}

void RootFileSystem::Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs)
{
    if (FindMountPoint(path, nullptr, nullptr))
    {
        LOG_ERROR("Cannot mount a filesystem at {}: there is another FS that includes this path", path.generic_string());
        return;
    }

    m_MountPoints.push_back(std::make_pair(path.lexically_normal().generic_string(), fs));
}

void RootFileSystem::Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath)
{
    Mount(path, std::make_shared<RelativeFileSystem>(std::make_shared<NativeFileSystem>(), nativePath));
}

bool RootFileSystem::Unmount(const std::filesystem::path &path)
{
    std::string spath = path.lexically_normal().generic_string();

    for (size_t index = 0; index < m_MountPoints.size(); index++)
    {
        if (m_MountPoints[index].first == spath)
        {
            m_MountPoints.erase(m_MountPoints.begin() + index);
            return true;
        }
    }

    return false;
}

bool RootFileSystem::FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS)
{
    std::string spath = path.lexically_normal().generic_string();

    for (auto it : m_MountPoints)
    {
        if (spath.find(it.first, 0) == 0 && ((spath.length() == it.first.length()) || (spath[it.first.length()] == '/')))
        {
            if (pRelativePath)
            {
                std::string relative = (spath.length() == it.first.length()) ? "" : spath.substr(it.first.size() + 1);
                *pRelativePath = relative;
            }

            if (ppFS)
            {
                *ppFS = it.second.get();
            }

            return true;
        }
    }

    return false;
}

bool RootFileSystem::DirectoryExists(const std::filesystem::path& name)
{
	std::filesystem::path relativePath;
	IFileSystem* fs = nullptr;

	if (FindMountPoint(name, &relativePath, &fs))
	{
		return fs->DirectoryExists(relativePath);
	}

	return false;
}

bool RootFileSystem::FileExists(const std::filesystem::path& name)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (FindMountPoint(name, &relativePath, &fs))
    {
        return fs->FileExists(relativePath);
    }

    return false;
}

std::shared_ptr<IBlob> RootFileSystem::ReadFile(const std::filesystem::path& name)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (FindMountPoint(name, &relativePath, &fs))
    {
        return fs->ReadFile(relativePath);
    }

    return nullptr;
}

bool RootFileSystem::WriteFile(const std::filesystem::path& name, const void* data, size_t size)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (FindMountPoint(name, &relativePath, &fs))
    {
        return fs->WriteFile(relativePath, data, size);
    }

    return false;
}

int RootFileSystem::EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, enumerate_callback_t callback, bool allowDuplicates)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (FindMountPoint(path, &relativePath, &fs))
    {
        return fs->EnumerateFiles(relativePath, extensions, callback, allowDuplicates);
    }

    return status::PathNotFound;
}

int RootFileSystem::EnumerateDirectories(const std::filesystem::path& path, enumerate_callback_t callback, bool allowDuplicates)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (FindMountPoint(path, &relativePath, &fs))
    {
        return fs->EnumerateDirectories(relativePath, callback, allowDuplicates);
    }

    return status::PathNotFound;
}

static void AppendPatternToReges(const std::string &pattern, std::stringstream &regex)
{
    for (char c : pattern)
    {
        switch (c)
        {
        case '?': regex << "[^/]?"; break;
        case '*': regex << "[^/]+"; break;
        case '.': regex << "\\."; break;
        default: regex << c;
        }
    }
}

std::string GetFileSearchRegex(const std::filesystem::path &path, const std::vector<std::string> &extensions)
{
    std::filesystem::path normalizedPath = path.lexically_normal();
    std::string normalizedPathStr = normalizedPath.generic_string();

    std::stringstream regex;
    AppendPatternToReges(normalizedPathStr, regex);

    if (stringutils::ends_with(normalizedPathStr, "/") && !normalizedPath.empty())
        regex << '/';
    regex << "[^/]+";

    if (!extensions.empty())
    {
        regex << '(';
        bool first = true;
        for (const auto &ext : extensions)
        {
            if (!first)
                regex << '|';
            AppendPatternToReges(ext, regex);
            first = false;
        }
        regex << ')';
    }

    return regex.str();
}

}