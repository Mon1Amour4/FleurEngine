# Directional Cascaded Shadow Stability Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate abrupt directional-shadow disappearance during camera movement by making cascade coverage observable, cascade transitions continuous, and light projections stable.

**Architecture:** Keep cascade selection in camera/view-space depth. Keep one 2D-array shadow map layer per cascade and the existing layered geometry pass. First prove whether failures are caused by layer selection or light-projection coverage; then add overlap blending, texel snapping, and tighter caster-aware depth bounds without changing the established descriptor contract.

**Tech Stack:** Vulkan, GLSL 450, SPIR-V, Fleur Vulkan backend, C++ math types, existing F2/F3/F4 debug controls.

---

## Current Contract and Findings

The following behavior is intentional and must remain:

```text
camera projection/view
  -> inverse(proj * view)
  -> world-space frustum corners
  -> camera-depth cascade splits
  -> one light view/projection per split
  -> layered depth rendering with gl_Layer
  -> fragment selection by camera view-space depth
```

The current UBO contract is 1072 bytes and must not be changed without updating both C++ and GLSL:

```text
offset 0     mat4 lightSpaceMatrices[16]
offset 1024  uint cascadeCount + std140 padding
offset 1040  vec4 cascadeSplits[2]
```

Confirmed defects:

1. `SelectCascade()` chooses exactly one layer and there is no transition blend.
2. `BuildDirectionalShadowFrustum()` moves the light projection with the camera and has no texel snapping.
3. `ShadowCalculation()` treats a projection coverage miss as fully lit, hiding whether the matrix or layer is wrong.
4. `zMult = 10` expands light-space depth excessively and reduces depth precision.
5. Global material shadow flags are combined with arithmetic addition instead of bitwise OR.
6. Shader source and tracked SPIR-V can become stale because compilation is not enforced deterministically.

Microsoft documents the same CSM trade-off: fit-to-cascade changes projection size/orientation with the camera, while fixed-size fitting and texel-sized movement reduce shimmering. Microsoft also recommends blending in a band between cascades:

- https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps
- https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps

## Files and Responsibilities

- Modify `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp` for split matrices, texel snapping, caster-aware bounds, shadow extent, and flag merging.
- Modify `Engine/Fleur/Lux/Vulkan/FVkDepthTarget.hpp` and `Engine/Fleur/Lux/Vulkan/FVkDepthTarget.cpp` to expose the configured shadow extent when required by the renderer.
- Modify `Sandbox/Resources/Shaders/opaqueFragment.frag` for diagnostic coverage output and cascade overlap blending.
- Recompile every affected tracked module from its GLSL source, including `opaqueVertex.spv`, `opaqueFragment.spv`, `shadowVertex.spv`, `shadowGeometry.spv`, and `shadowFragment.spv` when their interfaces are rebuilt.
- Modify `Sandbox/Resources/Shaders/compile.bat` so compilation is relative to the script directory and fails on any shader error.
- Add focused CPU tests under `Tests/` after extracting pure frustum/snap helpers from private renderer code; wire the tests into `Tests/CMakeLists.txt`.

## Task 1: Add deterministic cascade diagnostics

**Files:**
- Modify: `Sandbox/Resources/Shaders/opaqueFragment.frag`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Add a diagnostic result enum in the fragment shader:

```glsl
const int SHADOW_STATUS_OK = 0;
const int SHADOW_STATUS_OUTSIDE_XY = 1;
const int SHADOW_STATUS_OUTSIDE_Z = 2;
```

- [ ] Make the shadow function return a status through an `out int status` parameter instead of silently returning lit for out-of-bounds coordinates.
- [ ] Preserve F3 cascade colors and add distinct diagnostic colors for outside-XY and outside-Z. Do not mix diagnostic colors into normal lighting when F3 is disabled.
- [ ] Ensure the debug color uses the same `fragmentCameraDepth` passed to cascade selection.
- [ ] Capture screenshots with an encoded visible diagnostic color. Do not require shader-side logging: the current fragment stage only writes `outColor` and has no readback buffer.
- [ ] For a receiver test, record `fragmentCameraDepth` and split values. A fixed world-space receiver is valid only if its camera-space depth remains inside one interval; otherwise classify a split crossing as expected behavior.

Expected result: the next investigation can distinguish hard layer switching from projection coverage loss.

## Task 2: Add cascade overlap and blending

**Files:**
- Modify: `Sandbox/Resources/Shaders/opaqueFragment.frag`

- [ ] Keep `SelectCascade()` as the primary interval-based selector.
- [ ] Define the overlap at split `i` in absolute camera-space units. For `i < cascadeCount - 1`, let `split = cascadeSplits[i]`, `previous = i == 0 ? cameraNear : cascadeSplits[i - 1]`, and `next = cascadeSplits[i + 1]`; set `overlapWidth = clamp(0.10 * (next - previous), 0.001, 0.49 * min(split - previous, next - split))`.
- [ ] Blend only for `depth ∈ [split - overlapWidth, split]`, using `weight = smoothstep(split - overlapWidth, split, depth)`. Outside that interval, use exactly one cascade. The final cascade has no next-cascade blend.
- [ ] When camera depth is in the overlap, sample cascade `i` and `i + 1` and linearly blend the shadow factors.
- [ ] Use the same PCF/noise mode and bias for both samples.
- [ ] Clamp the next index to `cascadeCount - 1`.
- [ ] Handle `cascadeCount == 0`, `cascadeCount == 1`, invalid homogeneous `w`, and both samples outside coverage explicitly. If only one sample is valid, use that sample; if neither is valid, emit the diagnostic coverage color and use the documented fallback lighting result.

Expected result: crossing a split produces a continuous shadow factor rather than a one-frame layer pop.

## Task 3: Stabilize each orthographic projection

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Preserve a stable per-cascade bounding-sphere extent rather than recomputing a changing `radius` as the only stability mechanism. Document that snapping removes sub-texel translation but cannot remove projection scale changes.
- [ ] Add an explicit directional shadow extent/resolution configuration instead of coupling the shadow image to the swapchain extent. Use the same extent for image creation, image-view recreation, render area, viewport, scissor, and texel-size calculation.
- [ ] Use independent X/Y texel sizes for a non-square target.
- [ ] Snap the light-space center before constructing the orthographic projection using nearest-texel rounding, with the sign convention tested for positive and negative coordinates:

```cpp
const float texelSizeX = (2.0f * halfWidth) / float(shadowWidth);
const float texelSizeY = (2.0f * halfHeight) / float(shadowHeight);
lightSpaceCenter.x = std::round(lightSpaceCenter.x / texelSizeX) * texelSizeX;
lightSpaceCenter.y = std::round(lightSpaceCenter.y / texelSizeY) * texelSizeY;
```

- [ ] Keep light direction/orientation fixed for a fixed directional-light input; only translation may follow the camera cascade center.
- [ ] Add a CPU test that moves the unsnapped center by less than one texel and verifies the snapped center is unchanged.

Expected result: small camera movements no longer move shadow edges continuously across texels.

## Task 4: Replace arbitrary Z expansion with caster-aware bounds

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] First retain receiver cascade corners and calculate their light-space X/Y/Z bounds.
- [ ] For each cascade, transform the renderer's world-space caster bounds into that cascade's light space and union them with the receiver bounds. If the scene has no global caster bounds, use a documented directional-light shadow distance and extrude/expand the caster bounds along the light direction.
- [ ] Apply the caster union to the actual cascade matrix construction; do not update only the unused global `m_DirectionalLightShadowFrustum`.
- [ ] Set orthographic left/right/bottom/top and near/far to contain both receiver and caster bounds plus explicit padding.
- [ ] Remove the ten-times expansion only after coverage diagnostics prove the per-cascade caster bounds are included.
- [ ] Keep depth bias independent from this task; do not compensate for a bad Z range by increasing bias.

Expected result: shadow casters are not clipped, while depth precision is materially better than the current `zMult = 10` range.

## Task 5: Correct flags and make shader binaries reproducible

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: `Sandbox/Resources/Shaders/compile.bat`
- Regenerate: `Sandbox/Resources/Shaders/opaqueVertex.spv`
- Regenerate: `Sandbox/Resources/Shaders/opaqueFragment.spv`
- Regenerate: `Sandbox/Resources/Shaders/shadowVertex.spv`
- Regenerate: `Sandbox/Resources/Shaders/shadowGeometry.spv`
- Regenerate: `Sandbox/Resources/Shaders/shadowFragment.spv` when its interface is rebuilt.

- [ ] Replace arithmetic flag merging with integer bitwise OR before converting to the push-constant representation, and apply the same merge to transparent draws.
- [ ] Make `compile.bat` resolve paths with `%~dp0`, check every compiler invocation with `|| exit /b 1`, remove unconditional `pause`, and verify every expected output exists.
- [ ] Compile the current sources as GLSL 450; do not claim a GLSL 460 requirement unless all affected sources are intentionally migrated together.
- [ ] Validate every regenerated module with `spirv-val` and reflection/disassembly. Check descriptor sets 0–6 in the opaque pipeline, the shadow pipeline's set 1/binding 0, push-constant ranges, vertex/fragment interfaces, geometry invocation count, layer output, array strides, UBO offsets, descriptor counts, and buffer range.
- [ ] Record source and binary hashes in the verification output; do not hand-edit SPIR-V.

Expected result: runtime shader behavior always corresponds to the checked-in GLSL source.

## Task 6: Extract math tests and validate the layered Vulkan contract

**Files:**
- Create or modify the focused shadow/frustum math header and source under `Engine/Fleur/Lux/Vulkan/`.
- Modify: `Tests/CMakeLists.txt`
- Add: focused tests under `Tests/` for split interpolation, Vulkan NDC depth, nearest-texel snapping, and projection coverage.
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp` for UBO `static_assert`s if the type is defined there.
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp` and `Sandbox/Resources/Shaders/shadowGeometry.geom` only when validation exposes a contract mismatch.

- [ ] Extract pure helpers for `SplitFrustum`, texel snapping, and light-space coverage so they can be tested without constructing a Vulkan device.
- [ ] Test Vulkan NDC Z in `[0,1]`, near/far split endpoints, corner ordering, positive and negative snapping coordinates, near/far ordering after `orthoRH_ZO`, and receiver/caster points mapping inside `[0,1]`.
- [ ] Add C++ `static_assert`s for `sizeof(DirectionalShadowMatrices) == 1072`, `offsetof(lightSpaceMatrices) == 0`, `offsetof(cascadeCount) == 1024`, and `offsetof(cascadeSplits) == 1040` while the current eight-split UBO contract remains active.
- [ ] Validate the layered pass with Vulkan validation and a visible layer-color/depth test: image view type is `VK_IMAGE_VIEW_TYPE_2D_ARRAY`, image/view layer count equals configured cascade count, all layers transition, dynamic rendering uses the same `layerCount`, geometry invocation `i` writes `gl_Layer = i`, and the main pass samples layer `i` selected by camera depth.
- [ ] If the configured cascade count is changed, update image creation, image view, transitions, render layer count, geometry invocation limit, UBO count, split storage, descriptor range, and shader compilation as one change.

Expected result: the math and Vulkan contracts are independently testable, and a layer mismatch cannot be confused with a CSM algorithm error.

## Verification Checklist

- [ ] With F3 enabled, each camera-depth interval outside an overlap band maps to exactly one expected cascade color.
- [ ] Inside an overlap band, verify a controlled blend of exactly two adjacent cascade colors; at every split, the transition is driven by configured camera-space depth, not light-space Z.
- [ ] A receiver that remains in one interval keeps the same selected cascade while the camera rotates slightly.
- [ ] No outside-coverage diagnostic appears for geometry inside the union of that cascade's receiver and caster bounds.
- [ ] Crossing a split produces a continuous shadow result through the overlap band.
- [ ] F4 modes still produce hard, PCF, and PCF+noise behavior without changing cascade selection.
- [ ] Shadow casters outside the receiver but within the configured shadow distance still cast shadows.
- [ ] Shader compilation succeeds and SPIR-V reflection reports the existing descriptor contract.
- [ ] Vulkan validation confirms that the 2D-array image view, image layer count, layout transitions, dynamic-rendering `layerCount`, geometry `gl_Layer`, and sampled array layer agree for every configured cascade.
- [ ] If `cascadeCount` is raised above eight, expand `cascadeSplits` from two `vec4`s to four `vec4`s and update C++, GLSL, UBO size, reflection checks, and tests together. The current 1072-byte contract supports 16 matrices but only eight split values.
- [ ] Full C++ build is attempted. If blocked by the known MSBuild duplicate `Path`/`PATH` environment error, report that blocker separately rather than treating shader verification as a successful engine build.

## Review Gates

1. Do not implement blending until diagnostics prove whether the observed disappearance is a split transition, a coverage miss, or both.
2. Do not reduce Z padding until caster coverage is verified.
3. Do not change the UBO layout or descriptor sets; the current reflection audit shows those are coherent.
4. Every shader edit must be followed by deterministic SPIR-V regeneration and reflection.
5. Add `static_assert`s for the C++ UBO size and member offsets before changing the UBO contract.
