#include "command.hpp"

namespace ignite
{
    static CommandManager *s_Instance = nullptr;
    
    CommandManager::CommandManager()
    {
        s_Instance = this;
    }

    CommandManager *CommandManager::GetInstance()
    {
        return s_Instance;
    }
}