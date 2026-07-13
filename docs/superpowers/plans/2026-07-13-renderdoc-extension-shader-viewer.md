# RenderDoc Extension Shader Viewer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a RenderDoc extension that automatically inspects the selected capture/draw call and renders a clean shader value viewer without manual copy/paste.

**Architecture:** Keep the first version narrow: a small C++/Qt-based extension surface, a normalized capture snapshot model, a filtering layer that hides compiler-generated noise, and a UI that renders only curated shader data. Separate the extraction/filtering logic from the UI so the non-UI parts can be tested without RenderDoc.

**Tech Stack:** C++20, CMake, Qt-based RenderDoc extension UI, RenderDoc extension/contrib API, gtest for model/filter tests.

---

### Task 1: Scaffold the extension project and build wiring

**Files:**
- Create: `tools/RenderDocShaderViewer/CMakeLists.txt`
- Create: `tools/RenderDocShaderViewer/src/CMakeLists.txt`
- Create: `tools/RenderDocShaderViewer/src/RenderDocShaderViewerExtension.h`
- Create: `tools/RenderDocShaderViewer/src/RenderDocShaderViewerExtension.cpp`
- Create: `tools/RenderDocShaderViewer/src/RenderDocShaderViewerPlugin.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/ROADMAP.md` only if the new tool needs a roadmap entry later; do not do it in this task unless the build requires it.

- [ ] **Step 1: Add the build target declaration**

Create `tools/RenderDocShaderViewer/CMakeLists.txt` with a placeholder target layout that references the future sources but does not yet implement the extension:

```cmake
cmake_minimum_required(VERSION 3.24)

project(RenderDocShaderViewer LANGUAGES CXX)

add_subdirectory(src)
```

Create `tools/RenderDocShaderViewer/src/CMakeLists.txt` with the target skeleton:

```cmake
add_library(RenderDocShaderViewerExtension SHARED
    RenderDocShaderViewerPlugin.cpp
    RenderDocShaderViewerExtension.cpp
    RenderDocShaderViewerExtension.h
)

target_compile_features(RenderDocShaderViewerExtension PRIVATE cxx_std_20)
set_target_properties(RenderDocShaderViewerExtension PROPERTIES FOLDER "Tools")
```

At repo root, add the new tool subtree:

```cmake
add_subdirectory(tools/RenderDocShaderViewer)
```

- [ ] **Step 2: Run CMake configure and verify the new target is visible**

Run:

```powershell
cmake -S . -B Build\x64
```

Expected: configure reaches the new tool directory and includes the new target in the build graph.

- [ ] **Step 3: Add the minimal extension registration surface**

Create `tools/RenderDocShaderViewer/src/RenderDocShaderViewerPlugin.cpp`:

```cpp
#include "RenderDocShaderViewerExtension.h"

extern "C" bool RegisterRenderDocShaderViewerExtension()
{
    return true;
}
```

Create the extension header with the initial public surface:

```cpp
#pragma once

class RenderDocShaderViewerExtension
{
public:
    static bool Initialize();
};
```

Create the extension cpp with an explicit stub that fails clearly until the UI is added:

```cpp
#include "RenderDocShaderViewerExtension.h"

bool RenderDocShaderViewerExtension::Initialize()
{
    return true;
}
```

- [ ] **Step 4: Run a build and verify the stub target links**

Run:

```powershell
cmake --build Build\x64 --config Debug --target RenderDocShaderViewerExtension
```

Expected: the stub shared library target builds successfully before the RenderDoc-specific implementation is added.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt tools/RenderDocShaderViewer docs/superpowers/plans/2026-07-13-renderdoc-extension-shader-viewer.md
git commit -m "scaffold RenderDoc shader viewer tool"
```

### Task 2: Define the normalized capture model and filtering rules

**Files:**
- Create: `tools/RenderDocShaderViewer/src/model/ShaderValueNode.h`
- Create: `tools/RenderDocShaderViewer/src/model/ShaderValueNode.cpp`
- Create: `tools/RenderDocShaderViewer/src/model/ShaderCaptureSnapshot.h`
- Create: `tools/RenderDocShaderViewer/src/model/ShaderCaptureSnapshot.cpp`
- Create: `tools/RenderDocShaderViewer/src/filter/ShaderValueFilter.h`
- Create: `tools/RenderDocShaderViewer/src/filter/ShaderValueFilter.cpp`
- Create: `Tests/RenderDocShaderViewer/ShaderValueFilterTests.cpp`
- Modify: `Tests/CMakeLists.txt`
- Modify: `tools/RenderDocShaderViewer/src/CMakeLists.txt`

- [ ] **Step 1: Write the failing tests for filter behavior**

Create `Tests/RenderDocShaderViewer/ShaderValueFilterTests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "filter/ShaderValueFilter.h"

TEST(ShaderValueFilter, HidesCompilerTemporariesByDefault)
{
    ShaderValueNode node;
    node.name = "%tmp42";
    node.userDeclared = false;

    EXPECT_FALSE(ShaderValueFilter::ShouldShow(node));
}

TEST(ShaderValueFilter, KeepsUserDeclaredValuesVisible)
{
    ShaderValueNode node;
    node.name = "albedo";
    node.userDeclared = true;

    EXPECT_TRUE(ShaderValueFilter::ShouldShow(node));
}
```

- [ ] **Step 2: Run the test binary and verify it fails**

Run:

```powershell
cmake --build Build\x64 --config Debug --target Tests
ctest --test-dir Build\x64 --build-config Debug -R ShaderValueFilter
```

Expected: compile failure or failing assertions until the model/filter is implemented.

- [ ] **Step 3: Implement the normalized model**

Create `ShaderValueNode.h`:

```cpp
#pragma once

#include <string>
#include <vector>

struct ShaderValueNode
{
    std::string name;
    std::string typeName;
    std::string valueText;
    bool userDeclared{false};
    bool pinned{false};
    std::vector<ShaderValueNode> children;
};
```

Create `ShaderCaptureSnapshot.h`:

```cpp
#pragma once

#include <string>
#include <vector>
#include "ShaderValueNode.h"

struct ShaderCaptureSnapshot
{
    std::string captureName;
    std::string eventDescription;
    std::string stageName;
    std::vector<ShaderValueNode> roots;
};
```

Implement `ShaderValueFilter::ShouldShow(const ShaderValueNode&)` so it returns `true` for `userDeclared`, `pinned`, and known user-visible roots, and `false` for compiler-temp-style names such as `%tmp`, `tmp`, or SSA-like autogenerated labels.

- [ ] **Step 4: Run the tests and verify they pass**

Run:

```powershell
cmake --build Build\x64 --config Debug --target Tests
ctest --test-dir Build\x64 --build-config Debug -R ShaderValueFilter
```

Expected: the filter tests pass.

- [ ] **Step 5: Commit**

```bash
git add tools/RenderDocShaderViewer/src/model tools/RenderDocShaderViewer/src/filter Tests/RenderDocShaderViewer Tests/CMakeLists.txt tools/RenderDocShaderViewer/src/CMakeLists.txt
git commit -m "add shader value model and filters"
```

### Task 3: Add automatic extraction from RenderDoc selection state

**Files:**
- Create: `tools/RenderDocShaderViewer/src/extract/RenderDocSelectionAdapter.h`
- Create: `tools/RenderDocShaderViewer/src/extract/RenderDocSelectionAdapter.cpp`
- Create: `tools/RenderDocShaderViewer/src/extract/ShaderCaptureExtractor.h`
- Create: `tools/RenderDocShaderViewer/src/extract/ShaderCaptureExtractor.cpp`
- Modify: `tools/RenderDocShaderViewer/src/CMakeLists.txt`

- [ ] **Step 1: Define the adapter contract**

Create `RenderDocSelectionAdapter.h`:

```cpp
#pragma once

#include <optional>
#include "model/ShaderCaptureSnapshot.h"

class RenderDocSelectionAdapter
{
public:
    virtual ~RenderDocSelectionAdapter() = default;
    virtual std::optional<ShaderCaptureSnapshot> ReadCurrentSnapshot() = 0;
};
```

- [ ] **Step 2: Implement the extractor boundary**

Create `ShaderCaptureExtractor.h`:

```cpp
#pragma once

#include <memory>
#include "extract/RenderDocSelectionAdapter.h"

class ShaderCaptureExtractor
{
public:
    explicit ShaderCaptureExtractor(std::unique_ptr<RenderDocSelectionAdapter> adapter);
    std::optional<ShaderCaptureSnapshot> Update();

private:
    std::unique_ptr<RenderDocSelectionAdapter> m_Adapter;
};
```

Implement `Update()` as a thin pass-through that will later host RenderDoc API access. For this milestone it should return the adapter snapshot unchanged so the UI can already consume the shape.

- [ ] **Step 3: Add a fake adapter test**

Create `Tests/RenderDocShaderViewer/ShaderCaptureExtractorTests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "extract/ShaderCaptureExtractor.h"

class FakeAdapter final : public RenderDocSelectionAdapter
{
public:
    std::optional<ShaderCaptureSnapshot> ReadCurrentSnapshot() override
    {
        ShaderCaptureSnapshot snapshot;
        snapshot.captureName = "Frame 12";
        snapshot.stageName = "fragment";
        return snapshot;
    }
};

TEST(ShaderCaptureExtractor, ReturnsAdapterSnapshot)
{
    ShaderCaptureExtractor extractor(std::make_unique<FakeAdapter>());
    auto snapshot = extractor.Update();
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->captureName, "Frame 12");
}
```

- [ ] **Step 4: Run the tests and verify the contract works**

Run:

```powershell
cmake --build Build\x64 --config Debug --target Tests
ctest --test-dir Build\x64 --build-config Debug -R ShaderCaptureExtractor
```

Expected: the adapter contract test passes.

- [ ] **Step 5: Commit**

```bash
git add tools/RenderDocShaderViewer/src/extract Tests/RenderDocShaderViewer tools/RenderDocShaderViewer/src/CMakeLists.txt
git commit -m "add RenderDoc selection extraction boundary"
```

### Task 4: Build the clean UI and wire automatic refresh

**Files:**
- Create: `tools/RenderDocShaderViewer/src/ui/ShaderViewerWindow.h`
- Create: `tools/RenderDocShaderViewer/src/ui/ShaderViewerWindow.cpp`
- Create: `tools/RenderDocShaderViewer/src/ui/ShaderTreeModel.h`
- Create: `tools/RenderDocShaderViewer/src/ui/ShaderTreeModel.cpp`
- Modify: `tools/RenderDocShaderViewer/src/RenderDocShaderViewerPlugin.cpp`

- [ ] **Step 1: Define the tree model the UI will render**

Create `ShaderTreeModel.h`:

```cpp
#pragma once

#include <vector>
#include "model/ShaderValueNode.h"

class ShaderTreeModel
{
public:
    void SetRoots(std::vector<ShaderValueNode> roots);
    const std::vector<ShaderValueNode>& Roots() const;

private:
    std::vector<ShaderValueNode> m_Roots;
};
```

- [ ] **Step 2: Implement the tool window**

Create `ShaderViewerWindow.h`:

```cpp
#pragma once

#include <memory>
#include "extract/ShaderCaptureExtractor.h"
#include "ui/ShaderTreeModel.h"

class ShaderViewerWindow
{
public:
    explicit ShaderViewerWindow(std::unique_ptr<ShaderCaptureExtractor> extractor);
    void Refresh();

private:
    std::unique_ptr<ShaderCaptureExtractor> m_Extractor;
    ShaderTreeModel m_Model;
};
```

In `Refresh()`, pull the current snapshot, run filtering, and update the tree model. The UI should never inspect RenderDoc data directly.

- [ ] **Step 3: Initialize the tool window from the extension registration path**

Update `RenderDocShaderViewerPlugin.cpp` so the extension registration path creates the tool window, connects selection refresh, and hands control back to the host UI loop:

```cpp
// The plugin entry point constructs the window and registers it with RenderDoc.
// The host UI event loop continues to run inside RenderDoc.
```

The actual RenderDoc event hookup comes next, but this task establishes the UI shape and ownership boundaries. The plugin registration file is the place where RenderDoc will later create or attach the tool window.

- [ ] **Step 4: Add a smoke test for tree flattening**

Create `Tests/RenderDocShaderViewer/ShaderTreeModelTests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "ui/ShaderTreeModel.h"

TEST(ShaderTreeModel, StoresRoots)
{
    ShaderTreeModel model;
    model.SetRoots({ShaderValueNode{.name = "albedo", .userDeclared = true}});
    ASSERT_EQ(model.Roots().size(), 1u);
    EXPECT_EQ(model.Roots()[0].name, "albedo");
}
```

- [ ] **Step 5: Run the tests and verify the UI model behavior**

Run:

```powershell
cmake --build Build\x64 --config Debug --target Tests
ctest --test-dir Build\x64 --build-config Debug -R "Shader(TreeModel|CaptureExtractor|ValueFilter)"
```

Expected: model and filter tests pass.

- [ ] **Step 6: Commit**

```bash
git add tools/RenderDocShaderViewer/src/ui tools/RenderDocShaderViewer/src/RenderDocShaderViewerPlugin.cpp Tests/RenderDocShaderViewer
git commit -m "add shader viewer window and tree model"
```

### Task 5: Wire the extension to RenderDoc capture changes and polish MVP

**Files:**
- Modify: `tools/RenderDocShaderViewer/src/RenderDocShaderViewerExtension.cpp`
- Modify: `tools/RenderDocShaderViewer/src/extract/ShaderCaptureExtractor.cpp`
- Modify: `tools/RenderDocShaderViewer/src/ui/ShaderViewerWindow.cpp`
- Modify: `tools/RenderDocShaderViewer/src/filter/ShaderValueFilter.cpp`
- Modify: `tools/RenderDocShaderViewer/src/CMakeLists.txt`

- [ ] **Step 1: Replace the fake adapter with the RenderDoc-backed adapter**

Implement the RenderDoc-facing adapter so `ReadCurrentSnapshot()` reads the currently selected capture and draw call context, then normalizes it into `ShaderCaptureSnapshot`.

The extractor should fill:
- capture name
- current event / draw call label
- shader stage name
- curated roots for uniforms, inputs, outputs, and selected bindings

- [ ] **Step 2: Apply the filter before the UI sees data**

Update `ShaderCaptureExtractor::Update()` to:
1. read the raw snapshot from the adapter
2. traverse each root node
3. remove compiler temps and unnamed intermediates
4. preserve pinned or user-declared values
5. return the filtered snapshot

- [ ] **Step 3: Add a search box and empty-state messaging**

Extend the window so it supports:
- text search against `name`, `typeName`, and `valueText`
- empty-state text when nothing is selected
- non-fatal error display when extraction fails

- [ ] **Step 4: Run a final integration build and the model tests**

Run:

```powershell
cmake --build Build\x64 --config Debug --target RenderDocShaderViewerExtension Tests
ctest --test-dir Build\x64 --build-config Debug -R "Shader(TreeModel|CaptureExtractor|ValueFilter)"
```

Expected: build succeeds and the tests stay green.

- [ ] **Step 5: Commit**

```bash
git add tools/RenderDocShaderViewer docs/superpowers/plans/2026-07-13-renderdoc-extension-shader-viewer.md
git commit -m "wire automatic RenderDoc shader viewer"
```
