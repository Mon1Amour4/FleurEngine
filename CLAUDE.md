# FleurEngine

C++ rendering engine with Vulkan and OpenGL backends. `Sandbox` is the demo app (renders Sponza).

## Skill to use

Any Vulkan or renderer work: invoke `Skill(skill="vulkan-cpp-engineering")` **first**.

It lives outside this repo, in the user's personal skills directory — not under version control here, so do not look for it in the tree. It covers synchronization primitive choice, GPU resource lifetime in C++, validation-layer error triage, and the RenderDoc capture + shader-debugging workflow verified against this engine specifically (working capture settings, how to reach the shader debugger, how to read values).

## Build and run

```bash
cmake --build Build/x64 --config Debug --target Sandbox --parallel
```

Incremental builds take ~15s. Binary lands at `Build/x64/Debug/bin/Sandbox.exe`.

The exe is a `WIN32` subsystem app: **it has no console**, so a stdout-only logger writes nowhere and piping to `tee` yields an empty file. To capture Vulkan validation output, point the layer at a file via `VK_LAYER_SETTINGS_PATH` (see the skill).

## Shaders

Source and compiled SPIR-V both live in `Sandbox/Resources/Shaders/`. The exe loads `.spv` from there directly at runtime via a relative path (`..\..\..\..\Sandbox\Resources\`), so **editing a shader needs no C++ rebuild** — just recompile the one shader:

```bash
"C:/VulkanSDK/1.4.335.0/Bin/glslangValidator.exe" -V -gVS -Od -S frag -o <name>.spv <name>.frag
```

`compile.bat` does all of them but ends in `pause`, so it hangs when run non-interactively — call `glslangValidator` directly instead. The `-gVS -Od` flags embed debug info, which is what lets RenderDoc show original GLSL rather than only disassembly. Keep them for debug work.

## Renderer layout

- `Engine/Fleur/Lux/` — renderer abstraction (`IRenderer.hpp`, `Lux.cpp`), backend-agnostic types in `Graphics.hpp` / `RenderViews.hpp`
- `Engine/Fleur/Lux/Vulkan/` — Vulkan backend. `Renderer_Vulkan.cpp` is the bulk of it; per-object wrappers are `FVk*`
- Frame flow: `beginFrame()` → `ExecuteShadowPass()` → `ExecuteGBufferPass()` → `ExecuteLightingPass()` → `SubmitFrame()`
- Passes emit `vkCmdBeginDebugUtilsLabelEXT` labels, so RenderDoc groups them by name — keep new passes labelled

## Design docs

`docs/superpowers/specs/` and `docs/superpowers/plans/`, dated filenames.

## Conventions

- Scratch, debug dumps and agent tooling output belong in `C:\Dev\fleur-scratch`, not in this tree. `.gitignore` enforces it for the known offenders.
- Do not commit unless asked. Never `git add -A` — this working tree routinely holds several unrelated in-progress features at once, and sweeping them into one commit is destructive to the history.
- Asset binaries (`Sandbox/Resources/Models`, `Images`) change often during experiments; leave them alone unless the task is about assets.
