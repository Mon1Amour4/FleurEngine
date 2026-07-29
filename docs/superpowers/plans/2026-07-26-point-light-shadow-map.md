# Point Light Shadow Map Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a Vulkan point-light shadow-map resource class with six-layer cube-compatible depth textures, a matrix UBO, descriptors, and a vertex/geometry/fragment graphics pipeline.

**Architecture:** `PointLightShadowMap` owns one cube-compatible depth image per configured point light, a separate 2D-array view for layered rendering, a cube view for sampling, one sampler, one UBO containing six matrices per light, descriptor set/layout, shader modules, and pipeline objects. Existing `FVkTexture` gains explicit image-view creation so the two view types can coexist.

**Tech Stack:** C++17, Vulkan dynamic rendering, SPIR-V shader modules supplied through `Fleur::Graphics::SFLShaderInfo`, existing `FVkBuffer`, `FVkPipeline`, and GLSL shaders.

---

### Task 1: Support explicit Vulkan image-view types

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkTexture.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkTexture.cpp`

- [ ] Add `CreateImageView(VkImageViewType viewType, uint32_t layerCount)` and keep the existing overload delegating to the current default behavior.
- [ ] Preserve image format, aspect, mip count, and base layer while selecting the requested view type.
- [ ] Compile the Vulkan backend.

### Task 2: Add point-light shadow-map ownership

**Files:**
- Create: `Engine/Fleur/Lux/Vulkan/PointLightShadowMap.h`
- Create: `Engine/Fleur/Lux/Vulkan/PointLightShadowMap.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/CMakeLists.txt`

- [ ] Define a constructor taking `uint32_t textureCount`, a `Create(...)` method taking device, physical device, extent, depth format, and three `SFLShaderInfo` values, plus `Destroy`, matrix update, and getters.
- [ ] Allocate one 2D cube-compatible depth image with six layers per point light and create both `VK_IMAGE_VIEW_TYPE_2D_ARRAY` and `VK_IMAGE_VIEW_TYPE_CUBE` views.
- [ ] Allocate a host-visible UBO containing `mat4 viewProjection[6]` per texture.
- [ ] Create a depth sampler, descriptor set layout, descriptor pool, and descriptor set for the UBO and cube shadow-map descriptors.
- [ ] Release resources in reverse dependency order.

### Task 3: Add the three-stage shadow pipeline

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkPipeline.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkPipeline.cpp`
- Create: `Sandbox/Resources/Shaders/pointLightShadow.vert`
- Create: `Sandbox/Resources/Shaders/pointLightShadow.geom`
- Create: `Sandbox/Resources/Shaders/pointLightShadow.frag`

- [ ] Extend the pipeline description with an optional geometry shader and geometry entry point.
- [ ] Build vertex, geometry, and fragment shader stages when supplied.
- [ ] Use depth-only dynamic rendering and triangle input for the shadow pipeline.
- [ ] Add the requested vertex shader body with SSBO binding 0 and push constants `modelIdx`/`nodeIdx`.
- [ ] Add a minimal geometry shader skeleton that emits six layers and leaves matrix details easy to replace.
- [ ] Add a minimal empty fragment shader suitable for depth-only rendering.

### Task 4: Verify

**Files:**
- No new test framework dependency.

- [ ] Run the project’s configured build for `VulkanBackend`.
- [ ] Run shader compilation if the repository shader toolchain is available.
- [ ] Inspect the diff for ownership, descriptor counts, and Vulkan handle cleanup errors.
