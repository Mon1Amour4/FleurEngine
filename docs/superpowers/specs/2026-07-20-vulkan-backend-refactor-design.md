# Vulkan Backend Resource and Ownership Refactor

## Goal

Improve the Vulkan backend's resource lifetime, frame ownership, resize behavior, and repeated utility code without changing the renderer's visual behavior or synchronization model that was recently fixed.

## Scope

The refactor covers only `Engine/Fleur/Lux/Vulkan` and the directly related shader/resource lifecycle. Existing unrelated working-tree changes remain untouched.

## Design

### 1. Correctness fixes

- Fix array allocation cleanup in fallback cubemap creation.
- Create shared samplers and descriptor-set layouts once, outside the per-frame initialization loop.
- Keep the number of frames in flight explicit and separate from swapchain image count.
- Add Vulkan result checking to immediate command-buffer submission paths.
- Preserve defensive assertions for frame and swapchain-image indexing.

### 2. Frame-owned resources

Resources indexed by `m_CurrentFrame` will be grouped under `Frame`, including the shadow-map render target and its shadow descriptor set. Swapchain-image-indexed render-finished semaphores remain a separate collection because their lifetime is tied to acquired swapchain images.

Overlay vertex buffers will be allocated according to frames in flight, and recording will use the same frame index contract.

### 3. Shadow offset texture and resize

The random offset texture will use a fixed sampling-pattern extent independent of the window size. Its descriptor-set layout remains immutable after pipeline creation. Resize will recreate only resources whose dimensions depend on the swapchain and will not invalidate pipeline layouts.

### 4. Ownership and abstractions

- Convert renderer-owned heap objects to RAII-compatible ownership where this can be done without changing public APIs.
- Make descriptor-set-layout builder ownership explicit.
- Keep Vulkan handle destruction in the owning class.
- Consolidate repeated image-layout transition and descriptor-image-write helpers only where the existing call sites have the same semantics.

### 5. Naming cleanup

Apply mechanical naming fixes for obvious typos such as `CreateImaveView`, `m_SwapcainImageCount`, and `mimMapLevel`. No behavior will be changed by naming-only edits.

## Validation

After each logical batch:

1. Build `VulkanBackend` and `Sandbox` in Debug configuration.
2. Run the available Vulkan validation-enabled executable when possible.
3. Check that no new validation errors appear during startup, resize, shadow rendering, and presentation.
4. Inspect the final diff and ensure unrelated working-tree changes are not staged.

## Non-goals

- No PBR or lighting changes.
- No changes to shadow filtering quality beyond preserving the current random offset texture behavior.
- No broad rewrite of all Vulkan wrappers in one step.
- No commit of unrelated existing modifications.
