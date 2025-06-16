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
using img_text_queue = std::deque<std::pair<std::shared_ptr<Fuego::Graphics::Image2D>, std::shared_ptr<Fuego::Graphics::Texture>>>;

struct Toolchain
{
    struct renderer
    {
        std::shared_ptr<Fuego::Graphics::Texture> (*load_texture)(std::shared_ptr<Fuego::Graphics::Image2D> img,
                                                                  const Fuego::Graphics::Device* device){nullptr};
        void (*update)(){nullptr};
        static img_text_queue images;
    };
    struct assets_manager
    {
        // void (*update)(){nullptr};
    };

    renderer _renderer;
    assets_manager _assets_manager;
};

struct PostLoadPipeline
{
    static std::shared_ptr<Fuego::Graphics::Texture> load_texture(std::shared_ptr<Fuego::Graphics::Image2D> img, const Fuego::Graphics::Device* device)
    {
        return images_ptr->emplace_back(std::make_pair(img, device->CreateTexture(img->Name()))).second;
    }
    static void update()
    {
        if (images_ptr->empty())
            return;

        img_text_queue valid_items;
        for (const auto [img, texture] : *images_ptr)
        {
            if (!img->IsValid())
            {
                valid_items.push_back({img, texture});
                continue;
            }
            texture->PostCreate(img);
        }
        images_ptr->swap(valid_items);
    }
    static img_text_queue* images_ptr;
};
}  // namespace Fuego::Pipeline
