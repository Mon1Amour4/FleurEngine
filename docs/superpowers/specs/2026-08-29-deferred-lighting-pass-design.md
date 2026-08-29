# Deferred Lighting Pass Design

## Goal

Add a separate Vulkan deferred-lighting fragment shader. The existing `opaqueFragment.frag` remains responsible for the G-Buffer pass and is not changed to read from the G-Buffer it produces.

## Design

The new fullscreen Lighting Pass reads `gPosition`, `gNormal`, and `gAlbedoSpec` using screen UV coordinates and writes one final color output. The first implementation supports the existing directional-light data from the scene UBO and produces a basic diffuse/specular result. Shadow-map sampling is intentionally left as a follow-up integration once the light shadow bindings are unified.

The data flow is:

```text
opaqueFragment.frag + material textures
    -> gPosition, gNormal, gAlbedoSpec

deferredLighting.frag + G-Buffer + scene directional light
    -> finalColor
```

The shader will use a fullscreen triangle/quad vertex stage and a descriptor set containing the three G-Buffer samplers plus the scene data needed for the directional light. The Vulkan pass will transition the G-Buffer images to shader-read layout before drawing and render to the active color target.

## Scope and verification

- Add dedicated Lighting Pass shader source and compiled shader output using the repository's shader compilation flow.
- Add only the minimal Vulkan pipeline/pass wiring required to execute it.
- Keep the existing forward/main pass and shadow passes intact.
- Verify shader compilation and the project build; add focused tests only for testable CPU-side wiring because GPU output requires a runtime Vulkan validation path.

