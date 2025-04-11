#pragma once

namespace ignite
{
    class Scene;

    class Physics2D
    {
    public:
        Physics2D(Scene *scene);
        ~Physics2D();

        void Update();

    private:
        Scene *_scene;
    };
}