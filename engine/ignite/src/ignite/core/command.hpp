#pragma once

#include <functional>
#include <stack>

#include "types.hpp"

namespace ignite
{
    using CommandFunc = std::function<void()>;

    enum CommandState 
    { 
        CommandState_Create, 
        CommandState_Destroy,
        CommandState_Renaming
    };

    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
        virtual void Undo() = 0;
    };

    class CommandManager
    {
    public:
        CommandManager();
        
        static CommandManager *GetInstance();

        static void AddCommand(Scope<ICommand> command)
        {
            GetInstance()->m_UndoStack.push(std::move(command));
            GetInstance()->m_RedoStack = std::stack<Scope<ICommand>>();
        }

        static void ExecuteCommand(Scope<ICommand> command)
        {
            command->Execute();
            GetInstance()->m_UndoStack.push(std::move(command));
            GetInstance()->m_RedoStack = std::stack<Scope<ICommand>>();
        }

        void Undo()
        {
            if (m_UndoStack.empty())
            {
                return;
            }

            // store undo stack and pop it
            auto command = std::move(m_UndoStack.top());
            m_UndoStack.pop();
            command->Undo(); // execute undo command

            // push to redo
            m_RedoStack.push(std::move(command));
        }

        void Redo()
        {
            if (m_RedoStack.empty())
            {
                return;
            }

            // store redo stack and pop it
            auto command = std::move(m_RedoStack.top());
            m_RedoStack.pop();
            command->Execute(); // execute redo command

            // push to undo
            m_UndoStack.push(std::move(command));
        }

    private:
        std::stack<Scope<ICommand>> m_UndoStack;
        std::stack<Scope<ICommand>> m_RedoStack;
    };
}