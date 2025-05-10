#pragma once

#include <string>

namespace ignite {

    struct FileDialogs
    {
        static std::string OpenFile(const char *filter);
        static std::string SaveFile(const char *filter);
    };
}
