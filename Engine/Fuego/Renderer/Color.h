#pragma once

namespace Fuego::Graphics
{

class Color
{
   public:
    Color(uint8_t val, uint8_t val_2 = 0, uint8_t val_3 = 0, uint8_t val_4 = 0);
    ~Color() = default;

    static uint32_t Channels(const Color& color);
    uint32_t Data() const { return data; }

   private:
    uint32_t data;
};

}  // namespace Fuego::Graphics
