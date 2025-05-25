#pragma once

namespace Fuego::Graphics
{
class Image2D;
}

namespace Fuego::Pipeline
{
struct Toolchain
{
    struct renderer
    {
        void (*load_texture)(const Fuego::Graphics::Image2D& img){nullptr};
        std::queue<const Fuego::Graphics::Image2D*> images;
    };
    struct assets_manager
    {
        void (*post_load)(){nullptr};
    };

    renderer _renderer;
    assets_manager _assets_manager;
};

struct PostLoadPipeline
{
    static void load_texture(const Fuego::Graphics::Image2D& img)
    {
        FU_CORE_INFO("XUY");
        images_ptr->push(&img);
    }
    static void post_load()
    {
        int a = 5;
        int d = 5;
    }
    static std::queue<const Fuego::Graphics::Image2D*>* images_ptr;
};
}  // namespace Fuego::Pipeline