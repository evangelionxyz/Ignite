#pragma once

#include "ipanel.hpp"

namespace ignite {

    struct FileTree
    {

    };

    class ContentBrowserPanel : public IPanel
    {
    public:
        explicit ContentBrowserPanel(const char *windowTitle);

        virtual void OnGuiRender() override;

    private:

    };
}