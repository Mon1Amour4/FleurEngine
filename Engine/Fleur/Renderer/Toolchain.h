#pragma once
#include <memory>
#include <mutex>

#include "Device.h"
#include "Image2D.h"
#include "Texture.h"

using ImgTextPair = std::pair<std::shared_ptr<Fleur::Graphics::Image2D>, std::shared_ptr<Fleur::Graphics::Texture>>;

namespace Fleur
{

struct IRendererToolchain
{
    virtual ~IRendererToolchain() = default;
    virtual std::shared_ptr<Fleur::Graphics::Texture> LoadTexture(std::shared_ptr<Fleur::Graphics::Image2D> img, const Fleur::Graphics::Device* device) = 0;
    virtual void Update() = 0;
};

class PostLoadToolchain : public IRendererToolchain
{
public:
    PostLoadToolchain() = default;
    virtual ~PostLoadToolchain() override = default;

    std::shared_ptr<Fleur::Graphics::Texture> LoadTexture(std::shared_ptr<Fleur::Graphics::Image2D> img, const Fleur::Graphics::Device* device) override
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Pairs.emplace_back(std::make_pair(img, device->CreateTexture(img->Name(), img->Ext()))).second;
    }

    void Update() override
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto it = m_Pairs.begin(); it != m_Pairs.end();)
        {
            if (it->first->IsValid())
            {
                Fleur::Graphics::ImagePostCreation settings{it->first->Width(), it->first->Height(), it->first->Channels(), it->first->Depth(),
                                                            it->first->Data()};
                it->second->PostCreate(settings);
                it = m_Pairs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    std::list<ImgTextPair> m_Pairs;
    std::mutex m_Mutex;
};

}  // namespace Fleur
