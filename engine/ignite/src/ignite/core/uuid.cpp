#include "uuid.hpp"

#include <random>

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<u64> s_UniformDist;

namespace ignite
{
UUID::UUID()
: m_UUID(s_UniformDist(s_Engine))
{
}

UUID::UUID(const u64 uuid)
    : m_UUID(uuid)
{
}
}

