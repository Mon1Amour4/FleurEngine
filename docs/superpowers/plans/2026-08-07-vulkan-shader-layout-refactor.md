# Vulkan Shader Layout Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Separate shader reflection, pipeline layout creation, and graphics pipeline creation while preserving the current graphics stages and shader `set/binding` numbers.

**Architecture:** `FVkShader` owns shader modules and normalized reflection. `FVkPipelineLayout` owns the descriptor set layouts and the pipeline layout derived from all stages in one graphics shader program. `FVkPipeline` receives the ready layout and no longer accepts an independent descriptor-layout vector. Runtime descriptor arrays and frequency classification remain out of scope for this phase.

**Tech Stack:** C++20, Vulkan, SPIRV-Reflect, existing Fleur Vulkan backend, MSVC/CMake.

---

### Task 1: Normalize shader reflection

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkShader.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkShader.cpp`
- Test/verification: existing Vulkan shader initialization paths in `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [x] Add engine-owned reflection records for descriptor bindings, shader stages, push constants, and vertex input. Do not expose `VkDescriptorSetLayoutBinding` as the canonical reflection record.
- [x] Merge duplicate `(set, binding)` records while reflecting all stages. OR `stageFlags`; reject incompatible descriptor types or unsupported runtime/unsized arrays with a diagnostic error.
- [x] Preserve per-stage shader module, entry point, and stage information for pipeline creation.
- [x] Remove the `4096` fallback for zero-count combined image sampler arrays; fail initialization for unsupported runtime arrays.
- [x] Keep the current one-object graphics model: one `FVkShader` contains vertex, optional geometry, and fragment stages.
- [x] Delete the pipeline cache and `GetPipeline()` from `FVkShader` only after the new pipeline call path is ready in later tasks.
- [x] Make `FVkShader` non-copyable and define safe initialization/destruction for partial module creation.

### Task 2: Add `FVkPipelineLayout`

**Files:**
- Create: `Engine/Fleur/Lux/Vulkan/FVkPipelineLayout.h`
- Create: `Engine/Fleur/Lux/Vulkan/FVkPipelineLayout.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/CMakeLists.txt`

- [x] Add `FVkPipelineLayout::Init(VkDevice, const FVkShader&)` for the current one-program graphics model.
- [x] Build one descriptor set layout per set index, including valid empty layouts for gaps below the highest reflected set.
- [x] Merge stage bindings by `(set, binding)`, sort bindings deterministically, and validate type/count compatibility and device limits.
- [x] Normalize push constant ranges into non-overlapping ranges, merge stage flags, and validate alignment.
- [x] Own and destroy all descriptor set layouts and the pipeline layout exactly once; make the class non-copyable and explicitly non-movable.
- [x] Expose `Get()`, `GetSetLayout(set)`, and the ordered set-layout vector. `GetSetLayout` returns `VK_NULL_HANDLE` outside the vector range.

### Task 3: Make `FVkPipeline` consume the layout

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkPipeline.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkPipeline.cpp`
- Modify: all call sites found with `rg "FGraphicsPipelineDesc|\.GetPipeline\(" Engine/Fleur/Lux/Vulkan`

- [x] Remove `descriptorSetLayouts` from `FGraphicsPipelineDesc`.
- [x] Replace the optional raw `VkPipelineLayout` path with a required ready layout supplied by the caller, while keeping ownership outside `FVkPipeline`.
- [x] Ensure `FVkPipeline` never destroys descriptor set layouts or a layout it does not own.
- [x] Preserve existing render-state setup and dynamic-rendering formats.
- [x] Add the required shader stage data from `FVkShader` without storing dangling pointers to movable strings or vectors.

### Task 4: Migrate the opaque path first

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp` if ownership is stored there
- Modify: relevant opaque descriptor/pipeline setup files found by search

- [x] Construct one `FVkPipelineLayout` for the opaque shader program from reflection.
- [x] Allocate/use descriptor sets from `GetSetLayouts()` without changing shader `set/binding` declarations. Renderer-owned descriptor allocation is deferred until the reflected opaque layout exists.
- [x] Create opaque and transparent pipelines with the same layout only where their reflected interfaces are compatible.
- [x] Ensure the layout owner outlives both pipelines and all command submissions that reference them.
- [x] Verify that no old manually-created opaque pipeline layout is destroyed twice.

### Task 5: Move pipeline cache out of `FVkShader`

**Files:**
- Create or modify the backend-owned pipeline cache near `Engine/Fleur/Lux/Vulkan/FVkPipeline.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkDebugDraw.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkFloor.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkOverlayPass.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkSkybox.h`
- Modify: corresponding `.cpp` files and `Renderer_Vulkan.cpp`

- [x] Move cache ownership to a backend/render-resource owner whose lifetime includes all borrowed pipeline pointers.
- [x] Scope the cache to one shader/layout pair or include shader identity and layout identity in its key.
- [x] Migrate all current borrowed `FVkPipeline*` pointers without dangling references.
- [x] Remove `m_PipelineCache` and `GetPipeline()` from `FVkShader`.

### Task 6: Verification

**Files:**
- Verify: `Engine/Fleur/Lux/Vulkan/FVkShader.*`
- Verify: `Engine/Fleur/Lux/Vulkan/FVkPipelineLayout.*`
- Verify: `Engine/Fleur/Lux/Vulkan/FVkPipeline.*`

- [x] Run `git diff --check`.
- [ ] Reconfigure CMake for `Build/x64`.
- [ ] Build `VulkanBackend` and `OpenGLBackend` targets as available, then build `Sandbox` if the environment permits.
- [ ] Confirm diagnostics for conflicting duplicate bindings and unsupported runtime arrays.
- [ ] Confirm a shader using only set 2 produces set layouts 0, 1, and 2, with 0 and 1 empty.
- [x] Confirm no `VulkanMemoryAllocator` references are reintroduced and no unrelated dirty files are overwritten.
