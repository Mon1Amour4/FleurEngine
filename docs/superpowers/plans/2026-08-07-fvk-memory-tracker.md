# FVkMemoryTracker Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Centralize the existing vanilla Vulkan device-memory allocations behind `FVkMemoryTracker` and report allocated bytes plus physical memory-heap capacities without adding suballocation.

**Architecture:** `FVkMemoryTracker` is owned by `FVkDevice` and is initialized with the physical and logical devices. `FVkBuffer` and `FVkTexture` request and release `VkDeviceMemory` through it, passing an enum category instead of a debug string. The tracker records each allocation and prints aggregate statistics and physical heap capacities; it does not manage descriptor pools, query `VK_EXT_memory_budget`, or implement block/suballocation in this iteration.

**Tech Stack:** C++20, Vulkan-Hpp/C Vulkan headers already used by the backend, existing `FL_CORE_INFO`/`FL_CORE_ERROR` logging, CMake/Ninja with MSVC.

---

### Task 1: Define the tracker API and allocation categories

**Files:**
- Create: `Engine/Fleur/Lux/Vulkan/FVkMemoryTracker.h`
- Create: `Engine/Fleur/Lux/Vulkan/FVkMemoryTracker.cpp`

- [x] **Step 1: Add the public category enum and tracker interface**

Use a project-specific name so it is not confused with Vulkan's numeric memory-type index:

```cpp
enum class FVkAllocationCategory
{
    Buffer,
    Texture,
    RenderTarget,
    DepthTarget,
    Staging
};
```

The public API should be:

```cpp
class FVkMemoryTracker
{
public:
    void Init(VkPhysicalDevice physicalDevice, VkDevice device);

    VkDeviceMemory Allocate(const VkMemoryRequirements& requirements,
                            VkMemoryPropertyFlags properties,
                            FVkAllocationCategory category);

    void Free(VkDeviceMemory memory);

    void PrintStats() const;

private:
    struct AllocationInfo
    {
        VkDeviceSize size;
        uint32_t memoryTypeIndex;
        uint32_t heapIndex;
        FVkAllocationCategory category;
    };
};
```

- [x] **Step 2: Add build integration for the two new source files**

Add both files to the existing Vulkan backend target in `Engine/Fleur/Lux/Vulkan/CMakeLists.txt`, following the target's current source-list style.

- [x] **Step 3: Verify the API-only change compiles**

Run:

```powershell
cmd /d /c "call \"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat\" -arch=x64 -host_arch=x64 && cmake --build Build\Ninja --target VulkanBackend -j 1"
```

Expected: `VulkanBackend` builds; no resource classes have been migrated yet.

### Task 2: Implement direct allocation, release, and accounting

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkMemoryTracker.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkMemoryTracker.cpp`

- [x] **Step 1: Store physical-device heap information during `Init()`**

Call `vkGetPhysicalDeviceMemoryProperties` and store the memory types and heap sizes. In this iteration, “available memory” means the physical heap capacity reported by Vulkan; do not add `VK_EXT_memory_budget` or a second memory-properties query yet.

- [x] **Step 2: Implement memory-type selection and allocation**

Select the first index satisfying both `requirements.memoryTypeBits` and the requested property flags, with bounds checking against `memoryTypeCount`. If no index matches, log a descriptive error and fail the allocation. Call `vkAllocateMemory`, record the returned handle, allocation size, selected memory-type index, heap index, and category, then increment total, per-heap, and per-category counters.

- [x] **Step 3: Implement `Free()`**

Look up the handle in the tracker map, verify it belongs to this tracker, call `vkFreeMemory`, decrement the matching category, heap, and total counters, and erase the record. Treat a missing handle as a programming error and log it before returning in non-debug builds.

- [x] **Step 4: Implement readable statistics**

`PrintStats()` should print total allocated bytes, per-category bytes/counts, per-heap tracked bytes/counts, and each memory heap's capacity. Convert bytes to MiB only at formatting time. Label tracked bytes separately from heap capacity; the tracker is not claiming to report total physical usage.

- [x] **Step 5: Build the standalone tracker implementation**

Run the Vulkan backend build from Task 1. Expected: `VulkanBackend` builds successfully.

### Task 3: Migrate `FVkBuffer` to the tracker

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkBuffer.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkBuffer.cpp`
- Modify: all call sites of `FVkBuffer::Init` found with `rg -n "\.Init\(" Engine/Fleur`

- [x] **Step 1: Add a tracker reference/pointer to `FVkBuffer` initialization**

Extend `Init` with `FVkMemoryTracker& memoryTracker` and `FVkAllocationCategory category`. Store a pointer for later destruction. Delete copying and define move operations, or explicitly forbid moving after initialization, so the tracker pointer and `VkDeviceMemory` cannot be duplicated.

- [x] **Step 2: Replace direct allocation and release**

Replace `vkAllocateMemory` with `memoryTracker.Allocate(..., category)` and replace `vkFreeMemory` with `memoryTracker.Free(m_Memory)`. Keep `vkBindBufferMemory` in `FVkBuffer`. Preserve the Vulkan destruction order: unmap, destroy the buffer, then free its memory through the tracker.

- [x] **Step 3: Update buffer call sites**

Pass `FVkDevice::GetMemoryTracker()` at every `FVkBuffer::Init` call. Use `FVkAllocationCategory::Staging` for temporary upload/readback buffers and `FVkAllocationCategory::Buffer` for ordinary buffers. Do not alter descriptor allocation.

- [x] **Step 4: Build all affected targets**

Run:

```powershell
cmd /d /c "call \"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat\" -arch=x64 -host_arch=x64 && cmake --build Build\Ninja --target VulkanBackend Sandbox -j 1"
```

Expected: both targets compile and link.

### Task 4: Migrate `FVkTexture` and image resources

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/FVkTexture.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkTexture.cpp`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkDevice.h`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkDevice.cpp`
- Modify: image-resource call sites found with `rg -n "CreateImage\(" Engine/Fleur`

- [x] **Step 1: Own and expose the tracker from `FVkDevice`**

Add `FVkMemoryTracker m_MemoryTracker;` to `FVkDevice`, initialize it after the logical device exists, and expose:

```cpp
FVkMemoryTracker& GetMemoryTracker();
```

Declare the tracker before `m_StagingBuffers` so C++ member destruction cannot destroy the tracker before staging buffers. More importantly, explicitly clear all device-owned resources and wait for the GPU before `vkDestroyDevice`; do not rely only on member destruction order. All `FVkBuffer`/`FVkTexture` objects must be destroyed before the tracker and logical device.

- [x] **Step 2: Add tracker input and category to `FVkTexture::CreateImage`**

Extend the function with `FVkMemoryTracker& memoryTracker` and `FVkAllocationCategory category`, while preserving the existing image creation parameters. `FVkTexture::CreateImage()` must release an existing image before replacing the tracker pointer.

- [x] **Step 3: Replace image memory calls**

Use the tracker for allocation and release, retain image/image-view destruction in `FVkTexture`, and pass explicit categories at call sites: regular textures use `Texture`, color targets use `RenderTarget`, and depth/point-shadow targets use `DepthTarget`. Swapchain images are not tracked because their memory is owned by Vulkan.

- [x] **Step 4: Handle move operations safely**

Ensure the tracker pointer/reference association moves with `FVkTexture`, and clear the moved-from device, image, image view, memory, and tracker pointer so its destructor cannot free the same allocation twice.

- [x] **Step 5: Build the Vulkan backend and Sandbox**

Run the command from Task 3. Expected: successful compile and link.

### Task 5: Add teardown safety, runtime diagnostics, and validation

**Files:**
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp` or the device initialization owner, whichever owns the post-device startup sequence.

- [x] **Step 1: Print the initial memory report after device initialization**

Call `GetMemoryTracker().PrintStats()` once after the logical device and all initial resources are created, so the report includes actual engine allocations.

- [x] **Step 2: Add a final leak report before device destruction**

Before destroying the logical device, wait for GPU completion, destroy all renderer/helper resources and staging buffers, call `GetMemoryTracker().PrintStats()`, and verify that the live allocation count is zero. Only then call `vkDestroyDevice`. The tracker does not implement deferred destruction; callers must satisfy this synchronization contract.

- [ ] **Step 3: Run the application with Vulkan validation enabled**

Verify that buffers and images are reported under the expected categories, no duplicate frees occur, failed `vkBindBufferMemory`/`vkBindImageMemory` paths release both the Vulkan resource and tracker record, and existing descriptor-set validation remains unchanged.

- [x] **Step 4: Run final checks**

Run:

```powershell
git diff --check
cmd /d /c "call \"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat\" -arch=x64 -host_arch=x64 && cmake --build Build\Ninja --target VulkanBackend OpenGLBackend Sandbox -j 1"
```

Expected: no whitespace errors and all three targets build successfully.
