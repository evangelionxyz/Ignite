#include "command.hpp"

namespace ignite
{
    static CommandManager *s_JoltInstance = nullptr;
    
    CommandManager::CommandManager()
    {
        s_JoltInstance = this;
    }

    CommandManager *CommandManager::GetInstance()
    {
        return s_JoltInstance;
    }
}