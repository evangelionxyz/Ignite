#pragma once

#include <functional>
#include "types.hpp"

namespace ignite
{
class UUID
{
public:
    UUID();
    explicit UUID(u64 uuid);
    UUID(const UUID &uuid) = default;
    operator u64() const { return m_UUID; }
private:
    u64 m_UUID;
};
}

template<>
struct std::hash<ignite::UUID>
{
    std::size_t operator() (const ignite::UUID& uuid) const noexcept
    {
        return hash<u64>()(uuid);
    }
};

// template<>
// struct formatter<UUID>
// {
//     template<typename ParseContext>
//     constexpr auto parse(ParseContext &ctx)
//     {
//         return ctx.begin();
//     }

//     template<typename FormatContext>
//     auto format(const UUID &uuid, FormatContext &ctx) const
//     {
//         return fmt::format_to(ctx.out(), "{}", u64(uuid));
//     }
// };

