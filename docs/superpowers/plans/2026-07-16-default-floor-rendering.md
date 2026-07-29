# Default Floor Rendering Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a default scene floor and a renderer API backed by a dedicated Vulkan floor pipeline and shaders.

**Architecture:** The scene creates a persistent floor resource through the renderer API. The Vulkan implementation owns a small `FVkFloor` resource that creates a large textured plane, its descriptor resources, and a dedicated graphics pipeline; the floor is recorded in the main pass before model geometry.

**Tech Stack:** C++20, Vulkan dynamic rendering, GLSL 450 compiled to SPIR-V with `glslangValidator`, existing Fleur math and command/buffer wrappers.

---

### Task 1: Add the backend-neutral floor API

**Files:**
- Modify: `Engine/Fleur/Lux/RenderViews.hpp`
- Modify: `Engine/Fleur/Lux/IRenderer.hpp`
- Modify: `Engine/Fleur/Scene/Scene.h`
- Modify: `Engine/Fleur/Scene/Scene.cpp`

- [x] Add `CreateFloor` and `SetFloor` to the renderer interfaces.
- [x] Create the default floor from `Scene::Init()` using the loaded floor texture and height.

### Task 2: Implement the Vulkan floor resource

**Files:**
- Create: `Engine/Fleur/Lux/Vulkan/FVkFloor.h`
- Create: `Engine/Fleur/Lux/Vulkan/FVkFloor.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/CMakeLists.txt`
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.h`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [x] Create a large non-indexed plane vertex buffer and push-constant height.
- [x] Build a dedicated textured `FVkPipeline` using camera and floor texture descriptor layouts.
- [x] Add one-time `CreateFloor`/`SetFloor` configuration and persistent main-pass recording.
- [x] Record the floor in `ExecuteMainPass` only when the floor resource exists.

### Task 3: Add and compile floor shaders

**Files:**
- Create: `Sandbox/Resources/Shaders/floorVertex.vert`
- Create: `Sandbox/Resources/Shaders/floorFragment.frag`
- Modify: `Sandbox/Resources/Shaders/compile.bat`

- [x] Implement camera transform, plane normal, UV generation, and simple directional/ambient lighting.
- [x] Compile both shaders to `floorVertex.spv` and `floorFragment.spv`.

### Task 4: Verify integration

- [x] Configure/build the existing CMake Vulkan target.
- [x] Run the available test target to catch interface or compile regressions.
- [x] Verify shader compilation succeeded and generated both SPIR-V files.
