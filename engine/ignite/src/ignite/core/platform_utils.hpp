#pragma once

#include <string>
#include <vector>

namespace ignite {

    struct FileDialogs
    {
        static std::vector<std::string> OpenFiles(const char *filter);
        static std::string OpenFile(const char *filter);
        static std::string SaveFile(const char *filter);
    };
}
