# Directional Cascaded Shadow Geometry Shader Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Render the directional light shadow map into the configured number of array layers using one geometry-shader invocation per maximum cascade slot.

**Architecture:** `kCurrentCascadeCount` controls actual frustum, image-layer, barrier, and matrix counts. `kMaxCascadeCount` is a compile-time shader limit of 16. The vertex shader passes world positions to a geometry shader, which transforms each triangle with the cascade matrix and writes it to `gl_Layer`; the lighting fragment shader samples the selected array layer.

**Tech Stack:** Vulkan 1.x, GLSL 450, dynamic rendering, C++20.

---

### Task 1: Add shared cascade limits and matrix data

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/PrivateVulkanImpl.hpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Define `kMaxCascadeCount = 16` and `kCurrentCascadeCount = 5` near the renderer constants.
- [ ] Replace directional shadow resource and frustum uses of the old cascade constant with `kCurrentCascadeCount`.
- [ ] Change shadow push constants to carry `mat4 lightSpaceMatrices[16]` plus `uint cascadeCount`, while uploading only the active matrices and zero-initializing the remainder.

### Task 2: Add and compile the geometry shader

**Files:**
- Create: `Sandbox/Resources/Shaders/shadowGeometry.geom`
- Modify: `Sandbox/Resources/Shaders/shadowVertex.vert`
- Modify: `Sandbox/Resources/Shaders/compile.bat`

- [ ] Make the vertex shader output world position and keep object transforms in the vertex stage.
- [ ] Implement a `layout(triangles, invocations = 16) in` geometry shader with `layout(triangle_strip, max_vertices = 3) out`.
- [ ] Skip invocations at or above `cascadeCount`, transform each input vertex using `lightSpaceMatrices[gl_InvocationID]`, set `gl_Layer`, and emit one triangle.
- [ ] Compile the geometry shader and include its SPIR-V in shader loading.

### Task 3: Connect geometry shader to the shadow pipeline

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: shader loading declarations as required by the existing shader asset loader.

- [ ] Load the geometry shader bytecode for the directional shadow pass.
- [ ] Extend `createShadowPipeline` to accept the geometry shader and pass it to the graphics pipeline builder.
- [ ] Ensure dynamic rendering targets all active array layers and clears/stores the layered depth attachment.

### Task 4: Sample the directional shadow map as an array

**Files:**
- Modify: `Sandbox/Resources/Shaders/opaqueFragment.frag`
- Modify: `Sandbox/Resources/Shaders/opaqueVertex.vert`

- [ ] Change the descriptor type to `sampler2DArray`.
- [ ] Pass or calculate the active cascade index and sample with `vec3(uv, cascadeIndex)`.
- [ ] Keep point-light shadow samplers unchanged.

### Task 5: Verify

- [ ] Compile all affected GLSL shaders with `glslangValidator`.
- [ ] Build the Vulkan target with the existing CMake preset.
- [ ] Inspect git diff to ensure unrelated user changes and generated files are untouched.
