#pragma once

#include <array>
#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <regex>
#include <vector>

namespace ignite::stringutils
{
    inline bool EndsWith(std::string_view const &value, std::string_view const &ending)
    {
        if (ending.size() > value.size())
            return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin(), value.rend());
    }

    static std::string ToLower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    static void ReplaceWith(std::string &outStr, const std::string &targetKey, const std::string &replaceKey)
    {
        while (outStr.find(targetKey) != std::string::npos)
        {
            size_t targetPos = outStr.find(targetKey);
            outStr.replace(targetPos, targetKey.size(), replaceKey);
        }
    }
}
