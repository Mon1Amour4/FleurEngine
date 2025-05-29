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
        static std::deque<std::pair<std::shared_ptr<Fuego::Graphics::Image2D>, std::shared_ptr<Fuego::Graphics::Texture>>> images;
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
        images_ptr->emplace_back(std::make_pair(img, texture));
        return texture;
    }
    static void update()
    {
        for (auto it = (*images_ptr).begin(); it < (*images_ptr).end();)
        {
            auto img = (*images_ptr).front().first;
            if (!img->IsValid())
            {
                ++it;
                continue;
            }
            auto texture = (*images_ptr).front().second;
            texture->PostCreate(reinterpret_cast<const void*>(img.get()));
            it = (*images_ptr).erase(it);
        }
    }
    static void assets_manager_update()
    {
        int a = 5;
    }
    static std::deque<std::pair<std::shared_ptr<Fuego::Graphics::Image2D>, std::shared_ptr<Fuego::Graphics::Texture>>>* images_ptr;
};
}  // namespace Fuego::Pipeline