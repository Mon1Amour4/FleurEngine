#pragma once

#include "Device.h"
#include "Image2D.h"
#include "Texture.h"

namespace Fuego::Graphics
{
class Texture;
struct TexturePostCreation;
}  // namespace Fuego::Graphics

namespace Fuego::Pipeline
{
struct Toolchain
{
    struct renderer
    {
        std::shared_ptr<Fuego::Graphics::Texture> (*load_texture)(std::shared_ptr<Fuego::Graphics::Image2D> img,
                                                                  const Fuego::Graphics::Device* device){nullptr};
        void (*update)(){nullptr};
        static std::queue<std::pair<std::shared_ptr<Fuego::Graphics::Image2D>, std::shared_ptr<Fuego::Graphics::Texture>>> images;
    };
    struct assets_manager
    {
        void (*update)(){nullptr};
    };

    renderer _renderer;
    assets_manager _assets_manager;
};

struct PostLoadPipeline
{
    static std::shared_ptr<Fuego::Graphics::Texture> load_texture(std::shared_ptr<Fuego::Graphics::Image2D> img, const Fuego::Graphics::Device* device)
    {
        std::shared_ptr<Fuego::Graphics::Texture> texture = device->CreateTexture(img->Name());
        images_ptr->push(std::make_pair(img, texture));
        return texture;
    }
    static void update()
    {
        while (!images_ptr->empty())
        {
            auto item = images_ptr->front();
            auto img = item.first;
            FU_CORE_ASSERT(item.first->IsValid(), "[PostLoadPipeline->update] Image2D is not valid");

            item.second->PostCreate(*img.get());
            images_ptr->pop();
        }
    }
    static void assets_manager_update()
    {
        int a = 5;
    }
    static std::queue<std::pair<std::shared_ptr<Fuego::Graphics::Image2D>, std::shared_ptr<Fuego::Graphics::Texture>>>* images_ptr;
};
}  // namespace Fuego::Pipeline