# Deferred Lighting Shaders Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add fullscreen vertex and fragment shaders for the second deferred-rendering Lighting Pass.

**Architecture:** Keep `deferredFragment.frag` as the G-Buffer writer. Add a fullscreen triangle vertex shader and a fragment shader that samples the three G-Buffer attachments and calculates basic directional diffuse lighting. Vulkan pipeline wiring and shadow sampling are not part of this shader-only milestone.

**Tech Stack:** GLSL 4.50, Vulkan GLSL/SPIR-V, glslangValidator, spirv-val.

---

### Task 1: Add a compilation test script

**Files:**
- Create: `Tests/scripts/DeferredLightingShaders.tests.ps1`
- Test: `Tests/scripts/DeferredLightingShaders.tests.ps1`

- [ ] Write the script so it compiles `deferredLightingVertex.vert` as a vertex shader and `deferredLightingFragment.frag` as a fragment shader with `glslangValidator -V`, then validates both temporary outputs with `spirv-val`. It must throw when a tool or source shader is missing or when any command fails.
- [ ] Run `pwsh -File Tests/scripts/DeferredLightingShaders.tests.ps1` and verify it fails because the shader sources do not yet exist.

### Task 2: Add the fullscreen vertex shader

**Files:**
- Create: `Sandbox/Resources/Shaders/deferredLightingVertex.vert`
- Test: `Tests/scripts/DeferredLightingShaders.tests.ps1`

- [ ] Implement a three-vertex fullscreen triangle using `gl_VertexIndex`. Write UVs through `layout(location = 0) out vec2 texCoords`; use positions `(-1,-1)`, `(3,-1)`, `(-1,3)` and UVs `(0,0)`, `(2,0)`, `(0,2)`.
- [ ] Run the Task 1 test. Verify vertex compilation succeeds and the missing fragment shader causes the expected failure.

### Task 3: Add the Lighting Pass fragment shader

**Files:**
- Create: `Sandbox/Resources/Shaders/deferredLightingFragment.frag`
- Test: `Tests/scripts/DeferredLightingShaders.tests.ps1`

- [ ] Declare a screen UV input and `finalColor` output at location 0. Reflect the existing `SceneData` UBO at set 0/binding 0. Declare G-Buffer samplers at set 1 bindings 0, 1, and 2 for position, normal, and albedo/specular.
- [ ] Sample G-Buffer textures by screen UV, normalize the normal, calculate `max(dot(normal, -lightDirection), 0)`, multiply albedo by directional light color/intensity, and output it as `finalColor`. Do not add point lights, specular lighting, shadow maps, or pipeline wiring in this milestone.
- [ ] Run `pwsh -File Tests/scripts/DeferredLightingShaders.tests.ps1` and verify both shaders compile and validate.
- [ ] Run `cmd /c Sandbox\Resources\Shaders\compile.bat` and verify the complete shader directory compiles and validates.

### Task 4: Record the milestone

**Files:**
- Create: `Sandbox/Resources/Shaders/deferredLightingVertex.spv`
- Create: `Sandbox/Resources/Shaders/deferredLightingFragment.spv`
- Modify: `docs/superpowers/specs/2026-08-29-deferred-lighting-pass-design.md`

- [ ] Append to the design: `The shader-only milestone supplies a fullscreen triangle vertex shader and a directional-light fragment shader. Vulkan pipeline creation, descriptor allocation, swapchain rendering, point lights, and shadow-map sampling remain subsequent integration work.`
- [ ] Run `Get-Item Sandbox/Resources/Shaders/deferredLightingVertex.spv, Sandbox/Resources/Shaders/deferredLightingFragment.spv | Select-Object Name, Length` and verify both binaries are non-empty.
- [ ] Commit only the test script, new source and SPIR-V files, and the related design/plan documents with message `feat: add deferred lighting shaders`.
