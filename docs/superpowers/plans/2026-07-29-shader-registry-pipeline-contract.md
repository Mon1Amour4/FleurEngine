# Shader Registry and Pipeline Contract Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Load every compiled shader from the shader resource directory during `Application::Init`, register it through `AssetsManager`, and make renderer pipeline creation reference shader names instead of passing shader bytecode.

**Architecture:** `FileSystem` exposes a shader scan returning logical shader names and file paths. `Application` performs the scan and calls `AssetsManager::LoadShader` for each result before renderer backend selection. After the Vulkan/OpenGL backend and device are initialized, `Renderer` passes the registry to the backend; the backend resolves names, creates its shader objects, stores them by name, and uses only shader-name contracts for pipeline setup.

**Tech Stack:** C++17, `std::filesystem`, existing Fleur services, Vulkan/OpenGL renderer contracts, SPIR-V `.spv` assets.

---

### Task 1: Add filesystem shader scanning and explicit shader loading

**Files:**
- Modify: `Engine/Fleur/FileSystem/FileSystem.h`
- Modify: `Engine/Fleur/FileSystem/FileSystem.cpp`
- Modify: `Engine/Fleur/AssetsManager.h`
- Modify: `Engine/Fleur/AssetsManager.cpp`

- [ ] Add `FileSystem::ScanShaders()` returning deterministic `.spv` entries containing the stem name and full path. Ignore directories and sort by logical name.
- [ ] Add `AssetsManager::LoadShader(std::string_view logicalName, std::string_view path)` that reads binary bytes, allocates an `AssetID`, and inserts the shader into the existing name/id maps.
- [ ] Make `AssetsManager::OnInit()` stop scanning shaders implicitly; preserve the existing asset lookup API for callers.
- [ ] Keep duplicate names deterministic: reject/log the second registration instead of silently replacing the first asset.

### Task 2: Load the shader registry from Application initialization

**Files:**
- Modify: `Engine/Fleur/Application.cpp`
- Modify: `Engine/Fleur/Application.h`

- [ ] After FileSystem and AssetsManager services initialize, call `ScanShaders()` and load every returned entry through `AssetsManager::LoadShader`.
- [ ] Ensure this completes before `Renderer::SetBackend()` so backend shader creation sees the full registry.
- [ ] Keep runtime shader creation out of this change; this path is startup-only.

### Task 3: Change renderer contracts to shader names

**Files:**
- Modify: `Engine/Fleur/Lux/IRenderer.hpp`
- Modify: `Engine/Fleur/Lux/Lux.h`
- Modify: `Engine/Fleur/Lux/Lux.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.h`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`
- Modify: `Engine/Fleur/Lux/OpenGL/Renderer_OpenGL.h`
- Modify: `Engine/Fleur/Lux/OpenGL/Renderer_OpenGL.cpp`

- [ ] Replace `SFLShaderInfo`/`SFLShaderStages` pipeline inputs with a name-based stage descriptor, using empty names for unused stages.
- [ ] Add a renderer initialization input containing the shader registry from `AssetsManager`; after backend/device initialization, create backend shader objects into the backend name map.
- [ ] Make `CreatePass`, `CreateSkybox`, `CreateFloor`, `ConfigureOverlay`, and `ConfigureDebugDraw` resolve stage names through that map before forwarding to the backend.
- [ ] Remove `Renderer::shaderInfo()` and all direct shader-bytecode extraction from `AssetsManager` in `Lux.cpp`.
- [ ] Keep OpenGL compiling with the new interface even where shader stages are currently ignored.

### Task 4: Update Vulkan shader lookup and pipeline creation

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.h`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp`

- [ ] Replace `CreatePass`/skybox/floor/overlay/debug configuration arguments with named stages and resolve them through Vulkan's shader map.
- [ ] Keep one Vulkan shader object per logical shader name and report a clear error for missing names before pipeline creation.
- [ ] Preserve the existing point-light shadow face order, projection, viewport, and descriptor behavior.

### Task 5: Remove obsolete shader-loading path and verify

**Files:**
- Modify: `Sandbox/Resources/Shaders/compile.bat` only if output naming needs alignment.
- Modify: any build registration files required for the new source declarations.

- [ ] Remove obsolete implicit `load_all_shaders()` declarations/definitions after all callers use the Application scan.
- [ ] Build the affected Sandbox target.
- [ ] Verify that startup finds all `.spv` shader names and that the Vulkan pipeline setup uses names only.
- [ ] Run `git diff --check` and inspect the final staged file list.
