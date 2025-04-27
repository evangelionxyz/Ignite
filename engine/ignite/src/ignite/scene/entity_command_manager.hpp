#pragma once

#include "ignite/core/command.hpp"

namespace ignite
{
    class EntityManagerCommand : public ICommand
    {
    public:
        EntityManagerCommand(const CommandFunc &createFunc, const CommandFunc &destroyFunc, CommandState state)
        {
            m_CreateFunc = createFunc;
            m_DestroyFunc = destroyFunc;
            m_State = CommandState_Create;
        }

        virtual void Execute() override
        {
            switch (m_State)
            {
            case CommandState_Create:
            {
                m_CreateFunc();
                break;
            }
            case CommandState_Destroy:
            {
                m_DestroyFunc();
                break;
            }
            }
        }

        virtual void Undo() override
        {
            switch (m_State)
            {
            case CommandState_Create:
            {
                m_DestroyFunc();
                break;
            }
            case CommandState_Destroy:
            {
                m_CreateFunc();
                break;
            }
            }
        }

    private:
        CommandFunc m_CreateFunc;
        CommandFunc m_DestroyFunc;
        CommandState m_State;
    };
}