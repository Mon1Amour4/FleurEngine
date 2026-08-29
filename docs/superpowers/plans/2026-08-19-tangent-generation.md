# Tangent Generation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Import valid glTF tangents when available and generate robust fallback tangent frames when they are absent, so normal mapping works correctly without regressing shadow-pass performance.

**Architecture:** Keep tangent generation in `ModelFabric.cpp` at primitive-import time. Read all glTF attributes through `cgltf_accessor_read_float`, accumulate per-output-vertex tangent/bitangent data for fallback generation, then finalize an orthonormal `Vec4` tangent. Keep shader-space conversion separate: normals use inverse-transpose world transforms, tangents use the world linear transform, and the fragment shader reconstructs the handed bitangent.

**Tech Stack:** C++17, cgltf, Fleur math types, Vulkan GLSL 450, SPIR-V reflection, existing CMake/Visual Studio tests.

---

### Task 1: Add importer test fixtures and tangent-frame helpers

**Files:**
- Create: `Engine/Fleur/Lux/TangentGeneration.hpp`
- Create: `Engine/Fleur/Lux/TangentGeneration.cpp`
- Create: `Tests/src/TangentGenerationTests.cpp`
- Modify: `Engine/Fleur/Lux/CMakeLists.txt`
- Modify: `Tests/CMakeLists.txt`

- [ ] **Step 1: Write failing tests for determinant and orthonormalization**

Test a triangle with normal UVs and assert that the generated tangent is finite,
normalized, orthogonal to the normal, and has the expected handedness. Add a
mirrored-UV test whose negative UV determinant still produces a valid tangent
and the opposite handedness.

- [ ] **Step 2: Run the focused test and verify it fails**

Run:

```powershell
cmake --build Build\x64 --config Debug --target FleurTests
Build\x64\Tests\Debug\bin\FleurTests_d.exe --gtest_filter=TangentGeneration*
```

Expected: the new tangent tests fail because the helper does not exist yet.

- [ ] **Step 3: Implement the small, pure tangent helper**

Expose a function with this contract:

```cpp
struct TangentFrame
{
    Fleur::Vec3 tangent{0.0f};
    Fleur::Vec3 bitangent{0.0f};
    float handedness{1.0f};
    bool valid{false};
};

TangentFrame AccumulateTriangleTangent(
    Fleur::Vec3 p0, Fleur::Vec3 p1, Fleur::Vec3 p2,
    Fleur::Vec2 uv0, Fleur::Vec2 uv1, Fleur::Vec2 uv2,
    float epsilon = 1e-8f);

std::optional<Fleur::Vec4> FinalizeTangent(
    Fleur::Vec3 normal, Fleur::Vec3 accumulatedTangent,
    Fleur::Vec3 accumulatedBitangent);
```

Use `abs(det) <= epsilon` before division. For fallback, choose the least
aligned axis and project it with `axis - normal * dot(normal, axis)`. Return
`w = +1` whenever the tangent itself is synthesized. Return `std::nullopt`
when the normal is invalid so the importer can reject the primitive.

- [ ] **Step 4: Run the focused test and verify it passes**

Run the same build and test command. Expected: all tangent helper tests pass,
including mirrored UVs and degenerate UVs without NaN/Infinity.

- [ ] **Step 5: Commit the pure helper and tests**

```powershell
git add Engine/Fleur/Lux/TangentGeneration.hpp Engine/Fleur/Lux/TangentGeneration.cpp Tests/src/TangentGenerationTests.cpp Engine/Fleur/Lux/CMakeLists.txt Tests/CMakeLists.txt
git commit -m "test: add tangent frame generation"
```

### Task 2: Make primitive attribute reads accessor-safe

**Files:**
- Modify: `Engine/Fleur/Lux/ModelFabric.cpp:235-345`
- Modify: `Engine/Fleur/Lux/ModelFabric.h`
- Modify: `Tests/src/ModelFabricTests.cpp`
- Modify: `Tests/CMakeLists.txt`

- [ ] **Step 1: Add failing coverage for interleaved and offset accessors**

Build a minimal cgltf fixture with position, normal, UV, and tangent accessors
using non-zero accessor offsets and interleaved byte strides. Assert that the
imported `SVertexData` values match the logical accessor values rather than the
raw buffer's first floats.

- [ ] **Step 2: Run the focused model-fabric test and verify it fails**

Run:

```powershell
cmake --build Build\x64 --config Debug --target FleurTests
Build\x64\Tests\Debug\bin\FleurTests_d.exe --gtest_filter=ModelFabric*
```

Expected: the current `reinterpret_cast<float*>` reader produces incorrect
values for the strided fixture.

- [ ] **Step 3: Replace raw pointer reads with `cgltf_accessor_read_float`**

Read each accessor component by logical vertex index. Validate the expected
component counts: position/normal are VEC3, UV is VEC2, tangent is VEC4. Reject
unsupported primitive modes and log a diagnostic for missing position data.
Support non-indexed triangle primitives by using `0..vertex_count-1` as source
indices. Keep the existing source-index-to-output-index map so duplicated
vertices can be introduced for tangent seams.

- [ ] **Step 4: Run model-fabric tests and verify they pass**

Run the focused command again. Expected: strided, offset, normalized, and
sparse accessor fixtures pass without invalid reads.

- [ ] **Step 5: Commit accessor-safe importing**

```powershell
git add Engine/Fleur/Lux/ModelFabric.cpp Engine/Fleur/Lux/ModelFabric.h Tests/src/ModelFabricTests.cpp Tests/CMakeLists.txt
git commit -m "fix: read gltf attributes through accessors"
```

### Task 3: Parse supplied tangents and generate fallback tangents

**Files:**
- Modify: `Engine/Fleur/Lux/ModelFabric.cpp:235-345`
- Modify: `Tests/src/ModelFabricTests.cpp`

- [ ] **Step 1: Add failing tests for supplied and generated tangents**

Cover four cases: valid glTF tangent preservation after orthonormalization,
missing tangent fallback generation, missing UV fallback tangent with `w=+1`,
and invalid/missing normals producing generated normals or a clean primitive
rejection. Add a mirrored-UV case that checks `Tangent.w == -1` where expected.

- [ ] **Step 2: Run tests and verify the new cases fail**

Run:

```powershell
Build\x64\Tests\Debug\bin\FleurTests_d.exe --gtest_filter=ModelFabric*Tangent*
```

Expected: imported vertices currently contain the default `(1,0,0,1)` tangent.

- [ ] **Step 3: Implement per-primitive tangent accumulation**

Allocate tangent and bitangent accumulators for the output vertices. For every
triangle, read positions and the UV set associated with the material's normal
texture, calculate the determinant, skip only `abs(det) <= epsilon`, and add
the triangle contribution to all three output vertices. For supplied tangents,
read xyz/w and use them as the per-vertex input after finite/length validation.

- [ ] **Step 4: Finalize every output vertex**

Generate area-weighted normals if the normal accessor is absent. Normalize the
normal, orthogonalize the tangent against it, compute handedness with
`dot(cross(N,T), accumulatedB)`, and write `SVertexData::Tangent`. Split output
vertices when adjacent source corners require different handedness or tangent
directions; never interpolate incompatible frames.

- [ ] **Step 5: Run all importer tests and verify they pass**

Run the full Lux test target. Expected: supplied tangents, fallback tangents,
mirrored UVs, degenerate UVs, missing UVs, missing normals, and non-indexed
triangles all pass.

- [ ] **Step 6: Commit tangent import and fallback generation**

```powershell
git add Engine/Fleur/Lux/ModelFabric.cpp Tests/src/ModelFabricTests.cpp
git commit -m "feat: import and generate model tangents"
```

### Task 4: Correct shader transforms and material gating

**Files:**
- Modify: `Sandbox/Resources/Shaders/opaqueVertex.vert:67-71`
- Modify: `Sandbox/Resources/Shaders/opaqueFragment.frag:234-246`
- Modify: `Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp:1815-1837`
- Modify: `Engine/Fleur/Lux/Vulkan/FVkShader.cpp`
- Modify: `Engine/Fleur/Lux/Graphics.hpp`

- [ ] **Step 1: Add shader validation checks**

Compile opaque vertex and fragment shaders with `glslangValidator -V -Od` and
validate them with `spirv-val`. Add a static layout check:

```cpp
static_assert(sizeof(SVertexData) == 48);
static_assert(offsetof(SVertexData, Position) == 0);
static_assert(offsetof(SVertexData, TexCoord) == 12);
static_assert(offsetof(SVertexData, Normal) == 20);
static_assert(offsetof(SVertexData, Tangent) == 32);
```

- [ ] **Step 2: Implement correct vertex-space transforms**

Use the full `world = model * node` matrix. Transform normal with
`transpose(inverse(mat3(world)))`; transform tangent with `mat3(world)`. Pass
the world determinant sign to the fragment basis through tangent handedness or
an equivalent varying.

- [ ] **Step 3: Gate normal-map sampling safely**

Sample the normal texture only if F4 is enabled and the material has one. Do
not put `-1` into a `uvec4`; use a valid fallback texture index plus an explicit
availability flag. Keep the existing geometric-normal path when disabled.

- [ ] **Step 4: Make model vertex layout explicit**

Replace active-input stride inference for model pipelines with a shared explicit
layout containing `offsetof` offsets and `sizeof(SVertexData)` stride. Shadow
pipelines consume locations 0..2 without fetching tangent but still use stride
48. Opaque consumes location 3 as tangent.

- [ ] **Step 5: Compile and validate all affected shaders**

Run:

```powershell
Sandbox\Resources\Shaders\compile.bat
```

Then run `spirv-val` on opaque, directional-shadow, and point-light-shadow
SPIR-V files. Expected: all shaders validate, opaque has tangent location 3,
and shadow shaders have no tangent input.

- [ ] **Step 6: Commit shader and layout changes**

```powershell
git add Sandbox/Resources/Shaders Engine/Fleur/Lux/Vulkan/FVkShader.cpp Engine/Fleur/Lux/Graphics.hpp Engine/Fleur/Lux/Vulkan/Renderer_Vulkan.cpp
git commit -m "fix: use generated tangents in normal mapping"
```

### Task 5: End-to-end regression verification

**Files:**
- Modify: `docs/superpowers/specs/2026-08-19-tangent-generation-design.md` only if implementation behavior differs from the approved design.

- [ ] **Step 1: Build the complete Debug target**

```powershell
cmake --build Build\x64 --config Debug --parallel 4
```

Expected: exit code 0. If MSBuild reports the existing duplicate `Path/PATH`
environment failure, rerun from a clean Visual Studio developer environment.

- [ ] **Step 2: Run importer and shader validation tests**

Run the full Lux test executable and the shader compilation/validation commands
from Task 4. Expected: zero test failures and zero SPIR-V validation errors.

- [ ] **Step 3: Perform the runtime A/B check**

Run the Sandbox with F4 off and on. Confirm that F4 changes normal-mapped
lighting, materials without normal maps remain valid, and FPS stays near the
known 100 FPS baseline. Confirm the VertexLayout logs report full stride 48 for
model pipelines and no tangent fetch for shadow pipelines.

- [ ] **Step 4: Review the final diff and commit integration**

```powershell
git diff --check
git status --short
git commit -m "feat: robust tangent-space normal mapping"
```
