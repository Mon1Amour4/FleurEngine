// Debug-only helpers for natvis visualizers (Engine/Fleur/Fleur.natvis).
//
// natvis cannot compute sqrt / atan2 or zero-pad hex, so it can't derive a few
// values on its own. These free functions do the math; the visualizers func-eval
// them while you inspect a value. Compiled straight into Sandbox.exe so the
// debugger always resolves the symbol -- a static-lib object would be archive-
// stripped when nothing references it. Debug builds keep them (/OPT:NOREF);
// not present in Release.

#include <Lux/Color.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Fleur::Debug
{

// ===== glm::mat4 (affine TRS assumed; meaningless for a projection matrix) =====

// Per-axis scale = length of each basis column.
glm::vec3 FLScaleOf(const glm::mat4& m)
{
    return glm::vec3(glm::length(glm::vec3(m[0])),
                     glm::length(glm::vec3(m[1])),
                     glm::length(glm::vec3(m[2])));
}

// Euler angles in degrees (XYZ) of the rotation part, with scale divided out.
glm::vec3 FLEulerOf(const glm::mat4& m)
{
    const float lx = glm::length(glm::vec3(m[0]));
    const float ly = glm::length(glm::vec3(m[1]));
    const float lz = glm::length(glm::vec3(m[2]));

    const glm::mat3 rot(lx > 0.f ? glm::vec3(m[0]) / lx : glm::vec3(1, 0, 0),
                        ly > 0.f ? glm::vec3(m[1]) / ly : glm::vec3(0, 1, 0),
                        lz > 0.f ? glm::vec3(m[2]) / lz : glm::vec3(0, 0, 1));

    return glm::degrees(glm::eulerAngles(glm::quat_cast(rot)));
}

// ===== Color =====

// Packed 0xRRGGBBAA (HDR values clamped to [0;1] for the 8-bit view). Returned as a
// single uint so natvis can print it with the `,x` format and keep the middle digits;
// a per-channel hex would drop leading zeros and lose the channel boundaries.
unsigned FLColorHexRGBA(const Fleur::Graphics::Color& c)
{
    const glm::vec4 v = c.ToVec4();
    const unsigned R = static_cast<unsigned>(glm::clamp(v.r, 0.f, 1.f) * 255.f + 0.5f);
    const unsigned G = static_cast<unsigned>(glm::clamp(v.g, 0.f, 1.f) * 255.f + 0.5f);
    const unsigned B = static_cast<unsigned>(glm::clamp(v.b, 0.f, 1.f) * 255.f + 0.5f);
    const unsigned A = static_cast<unsigned>(glm::clamp(v.a, 0.f, 1.f) * 255.f + 0.5f);
    return (R << 24) | (G << 16) | (B << 8) | A;
}

}  // namespace Fleur::Debug
