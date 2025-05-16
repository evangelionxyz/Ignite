#include "platform_utils.hpp"

#include "application.hpp"

#ifdef PLATFORM_WINDOWS
#   include <Windows.h>
#   include <commdlg.h>
#   include <GLFW/glfw3.h>
#   include <GLFW/glfw3native.h>
#elif PLATFORM_LINUX
#   include <iostream>
#   include <memory>
#   include <stdexcept>
#   include <array>
#endif

namespace ignite {

#ifdef PLATFORM_WINDOWS

    std::vector<std::string> FileDialogs::OpenFiles(const char *filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[8192] = { 0 };
        CHAR currentDir[256] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window(Application::GetInstance()->GetWindow()->GetWindowHandle());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        if (GetCurrentDirectoryA(256, currentDir))
        {
            ofn.lpstrInitialDir = currentDir;
        }

        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

        std::vector<std::string> files;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            char *ptr = ofn.lpstrFile;

            std::string directory = ptr;
            ptr += directory.size() + 1;

            if (*ptr == '\0')
            {
                // Only one file was selected
                files.push_back(directory);
            }
            else
            {
                // Multiple files selected
                while (*ptr)
                {
                    std::string filename = ptr;
                    ptr += filename.size() + 1;
                    files.push_back(directory + "\\" + filename);
                }
            }
        }

        return files;
    }

    std::string FileDialogs::OpenFile(const char *filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        CHAR currentDir[256] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window(Application::GetInstance()->GetWindow()->GetWindowHandle());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
        {
            ofn.lpstrInitialDir = currentDir;
        }

        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return {};
    }

    std::string FileDialogs::SaveFile(const char *filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        CHAR currentDir[256] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window(Application::GetInstance()->GetWindow()->GetWindowHandle());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        // Sets the default extension by extracting it from the filter
        ofn.lpstrDefExt = strchr(filter, '\0') + 1;

        if (GetSaveFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return {};
    }

#elif PLATFORM_LINUX
    std::string ExecCommand(const char *cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }

        if (!result.empty() && result[result.length() - 1] == '\n')
        {
            result.erase(result.length() - 1);
        }
        return result;
    }

    std::string FileDialogs::OpenFile(const char *filter)
    {
        std::string command = "zenity --file-selection";
        if (filter)
        {
            command += " --file-filter=\"";
            command += filter;
            command += "\"";
        }
        std::string result = ExecCommand(command.c_str());
        if (result.empty())
        {
            return {};
        }
        return result;
    }

    std::string FileDialogs::SaveFile(const char *filter)
    {
        std::string command = "zenity --file-selection --save";
        if (filter)
        {
            command += " --file-filter=\"";
            command += filter;
            command += "\"";
        }
        std::string result = ExecCommand(command.c_str());
        if (result.empty())
        {
            return {};
        }
        return result;
    }
#endif
}