# Vulkan Backend Resource and Ownership Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task with verification checkpoints.

**Goal:** Fix Vulkan resource lifetime issues and make frame, resize, descriptor, and wrapper ownership clearer without changing rendering behavior.

**Architecture:** Keep swapchain-image resources separate from frame-in-flight resources. Move shadow maps into `Frame`, keep the random offset descriptor layout immutable, and use RAII for renderer-owned C++ objects where it does not alter Vulkan handles or pipeline layouts.

**Tech Stack:** C++17, Vulkan, VMA, CMake, Debug validation layers.

---

### Task 1: Establish baseline and isolate touched files

**Files:**
- Inspect: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Inspect: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp`
- Inspect: `Engine/Fleur/Lux/Vulkan/FVkCommand.cpp`
- Inspect: `Engine/Fleur/Lux/Vulkan/FVkPipeline.h`

- [ ] Record `git status --short` and keep the existing unrelated modifications unstaged.
- [ ] Build `VulkanBackend` and `Sandbox` in Debug configuration.
- [ ] Confirm the current working tree has no existing Vulkan build failure before edits.

### Task 2: Fix definite lifetime and initialization bugs

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Move `m_ShadowMapLayout = ...` before the `m_Frames` initialization loop.
- [ ] Move `m_ImageSampler = createTextureSampler()` before the loop.
- [ ] Replace `delete buffer;` with `delete[] buffer;` for the cubemap staging array.
- [ ] Keep one `m_ShadowMapLayout` and one `m_ImageSampler` owned by the renderer.
- [ ] Build Debug and verify no compile errors.

### Task 3: Make frame and swapchain counts explicit

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkOverlayPass.cpp`

- [ ] Introduce one named frames-in-flight constant and stop overwriting the swapchain count with `3`.
- [ ] Allocate overlay buffers using the frames-in-flight count passed to `Create`.
- [ ] Assert `frameIndex < m_Buffers.size()` in `FVkOverlayPass::Record`.
- [ ] Preserve render-finished semaphores indexed by acquired swapchain image.
- [ ] Build Debug and verify startup indexing checks remain valid.

### Task 4: Move shadow-map ownership into `Frame`

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Add `FVkDepthTarget shadowMap` to `Frame`.
- [ ] Remove the parallel `m_ShadowMapRenderTargets` vector.
- [ ] Create and recreate each frame's shadow map through `m_Frames[i]`.
- [ ] Update shadow descriptor sets and both passes to use `GetCurrentFrame().shadowMap`.
- [ ] Keep destruction ordered before VMA/device destruction.
- [ ] Build Debug and run validation-enabled startup if available.

### Task 5: Make random offset texture resize-safe

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/ShadowMapOffsetTexture.h`
- Modify: `Engine/Fleur/Lux/Vulkan/ShadowMapOffsetTexture.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Generate a fixed-size pattern texture independent of swapchain extent.
- [ ] Keep its descriptor-set layout and pipeline layout stable for the renderer lifetime.
- [ ] Remove resize dependence from `endResize`.
- [ ] Keep filter size in one C++ constant and one shader constant, with a documented matching contract.
- [ ] Build Debug and verify resize does not recreate the descriptor layout.

### Task 6: Improve command and descriptor utility boundaries

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkCommand.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/VkHelper.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/VkHelper.h`

- [ ] Add `VK_CHECK` to command-buffer end and queue-submit operations in immediate command paths.
- [ ] Consolidate only equivalent image-layout transition code paths.
- [ ] Keep depth, color, and sampled-image aspect/layout semantics explicit at call sites.
- [ ] Build Debug and check validation output for new errors.

### Task 7: Convert safe renderer ownership to RAII

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkPipeline.h`

- [ ] Convert renderer-owned heap objects to `std::unique_ptr` only where constructors/destructors are already complete and no external raw ownership is required.
- [ ] Change descriptor-layout builder ownership to `std::unique_ptr` and update call sites.
- [ ] Remove matching manual deletes after ownership conversion.
- [ ] Keep Vulkan handle destruction in the owning wrapper classes.
- [ ] Build Debug and inspect destruction order.

### Task 8: Mechanical cleanup and final verification

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkTexture.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkTexture.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkSwapchain.h`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Rename obvious typos such as `CreateImaveView`, `m_SwapcainImageCount`, and `mimMapLevel`.
- [ ] Add final frame/image bounds assertions where indexing crosses resource ownership boundaries.
- [ ] Build `VulkanBackend` and `Sandbox` in Debug configuration.
- [ ] Review `git diff --stat` and `git diff --check`.
- [ ] Confirm unrelated files remain unstaged and report the commit limitation if `.git` is still unwritable.
