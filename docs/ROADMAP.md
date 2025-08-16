## Fleur Engine – Refactoring & Evolution Road-map

> A living document that captures the **why**, **what**, and **order** of upcoming changes.  Keep it under version control and adjust as we learn.

---

### High-level goals
1. Multi-platform runtime (desktop **and** mobile – iOS & Android in the mid-term).
2. Strict binary size & start-up-time budgets.
3. Lean, testable core with room for **custom allocator experiments** (but **no custom smart-pointers**).
4. Plug-in architecture for render back-ends (OpenGL → Vulkan/Metal) & platform layers.
5. Integrated ImGui layer now; full **C# editor & scripting** later.
6. Packaged as a **git sub-module** for downstream games / tools.

---

### [Phase 0  Safety-net (weeks 1-2)](https://github.com/LtSnail/Fleur/issues/106)
| Step | Description | Win |
|------|-------------|-----|
| 0.1 | Enforce pre-commit, clang-format, clang-tidy in CI. | Consistent code & fast feedback |
| 0.2 | Add unit-test coverage badge and enable address-/UB-san in Debug builds. | Prevent regressions |
| 0.3 | Generate `compile_commands.json`; hook clangd for every contributor. | Better IDE experience |
| 0.4 | Stand-up documentation pipeline: Doxygen XML ➜ mdBook / Obsidian vault (markdown export). | Up-to-date, searchable docs |
| 0.5 | Connect AI helpers. | Faster dev & onboarding |
| 0.6 | Implement PIMPL base template. | Better module boundaries & C# integration |
| 0.7 | Polish Python project scripts: add YAML config, idempotent re-runs, optional `--gui` with Tkinter. | Better UX for project creation |

**Recommended reading**
• _Software Engineering at Google_ – Ch. 22 (Continuous Integration)
• GitHub Docs – "Actions best practices"
• clang.llvm.org – "clang-tidy quick start"
• *Keep a Changelog* (keepachangelog.com) – versioning & release notes
• _Large-Scale C++ Software Design_ – J. Lakos → Ch. 3 (Physical Design)
• Herb Sutter – "GotW #100: Compilation Firewalls"
• [PIMPL C++11 Library Documentation](https://yet-another-user.github.io/pimpl/doc/html/index.html) – Modern PIMPL implementation patterns

---

### AI & Documentation Strategy
_(runs in parallel with Phase 0 tasks)_

1. **Single-source docs**
   • Annotate public headers & CMake targets with Doxygen comments.
   • Generate Doxygen XML → feed into **mdBook** for a styled static site.
   • Export the same markdown tree into an **Obsidian** vault for offline note-taking and linking design docs.

2. **Continuous publishing**
   • Add a GitHub Actions job: on every `main` push build docs & deploy to **GitHub Pages**.
   • Attach artifact zip so teammates can drop the vault into Obsidian.

3. **IDE assistance with AI**
   • Enable **GitHub Copilot** for code completion; configure it to respect `.clang-format`.
   • Use a **ChatGPT** editor plug-in (e.g. *Continue* for VS Code) pre-configured with the local docs path so answers are context-aware.
   • For reviewers, add a pull-request template that auto-links to `clang-tidy` output and lets **Devin**/LLM generate a change-log and risk summary.

4. **Bot automation scripts**
   • `Scripts/ai/ask_llm.py` – CLI wrapper around OpenAI API that:
     – Locates relevant files via ripgrep.
     – Sends concise context with the prompt.
   • `Scripts/ai/doc_extractor.py` – combines Doxygen XML + inline `///` comments into a JSON chunk for LLM context.

5. **Pilot metrics**
   • Track Copilot acceptance rate & PR cycle time to validate ROI.
   • After three sprints, hold a retro: keep what helps, drop what slows us.

---

### Phase 1  Memory management – switch to STL pointers (weeks 2-3)
1. **Delete** `CoreLib/Sptr.h`, `CoreLib/Uptr.h`, `CoreLib/smart_ptr` tests and any includes.
2. Introduce a tiny alias header (e.g. `fu_pointers.hpp`) so we can write `Sptr<T>` → `std::shared_ptr<T>` & `Uptr<T>` → `std::unique_ptr<T>` while grepping.
3. Scripted refactor: replace all `Sptr/Uptr/Wptr` usages with the STL equivalents, compile, and run unit-tests.
4. Remove `FU_USE_STD_SMART_PTR` option; STL pointers are now mandatory.
5. Keep allocator R&D in `CoreLib/Allocator/` and use `std::allocate_shared` / `std::pmr` to plug experiments into the standard types.
6. **Expand PIMPL implementation** to Scene and Material classes, leveraging new STL pointers.

**Recommended reading**
• *Effective Modern C++* – Items 17–19 (smart-pointer rules)
• _CppCoreGuidelines_ – R.10, R.20 (resource safety)
• Herb Sutter – "Leaks? No. Exceptions? Yes!"  (ACCU 2017)

---

### Phase 2  C# Scripting Core & Chip-8 Demo (weeks 3-5)
Goal: Deliver a working C# scripting layer and use it to implement a Chip-8 emulator as a real-world test.

1. **Integrate .NET runtime** (NativeAOT or Mono) into engine core
2. **Expose minimal C++ API to C#**:
   - 2D graphics (DrawPixel, Clear, Present)
   - Input (key state)
   - Timer API (for 60Hz timers)
   - (Optional) Audio: simple beep
3. **Implement hot-reload for C# scripts** (file watcher + reload)
4. **Write Chip-8 emulator in C#** as a test project
5. **Document scripting API and Chip-8 example**

**Recommended reading**
• Microsoft Docs – "Hosting .NET Core in native apps"
• Mono Embedding API guide
• [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
• "Writing a Chip-8 Emulator" – Tobias V. Langhoff (blog)

---

### Phase 3  Platform abstraction + Swift migration (weeks 5-6)
1. **Define platform interfaces** – Extract common `IPlatform`, `IWindow`, `IEventQueue` from Win32/macOS code.
2. **Migrate macOS platform layer to Swift:**
   - `WindowMacOS.mm` → `WindowMacOS.swift` (~160 lines: NSWindow setup, view management)
   - `EventQueueMacOS.mm` → `EventQueueMacOS.swift` (~130 lines: NSEvent handling, notifications)
   - `EntryPointMacOS.mm` → `EntryPointMacOS.swift` (~40 lines: app lifecycle)
   - Keep `InputMacOS.cpp` as C++ (minimal, just key mapping)
3. **Create C++/Swift bridge patterns:**
   - Thin Objective-C++ bridge for complex C++ types (`std::shared_ptr<EventVariant>`)
   - `@objc` Swift classes that implement platform interfaces
   - SwiftPM package for shared iOS/macOS code
4. **Keep Metal backend in Objective-C++** – graphics performance-critical, ~20 files, complex C++ interop.
5. **Replace ServiceLocator** with constructor injection (parallel task).
6. **Add iOS platform stub** reusing macOS Swift patterns (~80% code reuse expected).

**Migration priority order:**
1. Window creation & lifecycle (lowest risk, high iOS reuse)
2. Event handling (moderate complexity, shared input patterns)
3. App entry point (simple, but different on iOS)
4. File system paths (trivial, good Swift learning project)

**Recommended reading**
• Apple Doc – *Swift Interoperability with C*
• WWDC 2023 – "Swift and C++: A Love Story" session video
• _Pro Swift_ – Ch. 6 (Protocols & Interop)
• objc.io – "Bridging C++ and Swift" article

---

### Phase 4  Policy-based core refactors (weeks 6-7)
Goal: Leverage C++20 templates & concepts to make low-level systems swappable at **compile-time** with zero runtime cost.

1. **Identify hot-spot subsystems** (allocators → logging → thread-pool).
2. **Define C++20 concepts** for each policy family.
3. **Create default + null policies**; integrate into `ThreadPool`, `AssetsManager`, `Log`.
4. **Benchmark** size & FPS impact (Google-benchmark).
5. **Document the matrix** in `docs/Policies.md` & Obsidian vault.

> Output: swappable policies selected via `using` aliases, e.g.
> `using EngineAllocator = policy::PoolAllocator<64_KB>;`

**Recommended reading**
• _Modern C++ Design_ – A. Alexandrescu → Chapters 1 (Policies) & 2 (Generic Components)
• ISO C++20 Standard – §20.5 (Memory resources) for `std::pmr`
• _C++ High Performance_ – B. Babich & D. Jost → Ch. 8 (Memory allocators)
• "Anatomy of a Thread Pool" – Folly docs (fbopensource.github.io/folly/)
• `spdlog` wiki – "Custom log sinks & compile-time log levels"
• Jason Turner – "Compile-time Logging with constexpr & Templates" (CppCon 2020)

---

### Phase 5  Cross-Platform Shader Compiler (weeks 7-8)
Goal: Create a unified shader compilation pipeline supporting multiple platforms and graphics APIs.

1. **Shader Compiler Architecture:**
   - Define common shader intermediate representation
   - Create platform-specific backends (HLSL, GLSL, Metal)
   - Implement shader validation and optimization

2. **Platform Support:**
   - Windows: DirectX (HLSL) and OpenGL (GLSL)
   - macOS/iOS: Metal
   - Android: OpenGL ES and Vulkan
   - Web: WebGL (future)

3. **Features:**
   - Shader preprocessing and includes
   - Cross-compilation between formats
   - Runtime shader reflection
   - Hot-reload support
   - Error reporting and debugging

4. **Integration:**
   - Build-time shader compilation
   - Runtime shader management
   - Asset pipeline integration

**Recommended reading**
• _Real-Time Rendering_ – T. Akenine-Möller → Ch. 3 (The Graphics Processing Unit)
• Khronos docs – "SPIR-V Specification"
• Microsoft Docs – "HLSL Shader Model 6.0"
• Apple Docs – "Metal Shading Language Guide"

---

### Phase 6  Project structure (weeks 8-9)
Create dedicated CMake targets:
```text
engine/
  core/           # Application, Events, LayerStack
  platform/       # Swift-based for Apple, C++ for others
      apple/      # Shared Swift code (macOS + iOS)
      win32/      # Windows C++ implementation
      android/    # Future Android C++/JNI
  renderer/       # API-agnostic front-end
  backends/
      opengl/
      vulkan/
      metal/      # Keep existing Objective-C++ Metal code
  assets/
  imgui_layer/
```
Each sub-dir builds into an interface/static library; link per-target to avoid global macros.

**Recommended reading**
• _Large-Scale C++ Software Design_ – J. Lakos → Ch. 5 (Physical design)
• Craig Scott – *Professional CMake* → Ch. 10–11 (Targets & Sub-projects)
• Google OSS Guide – "Bazel vs. CMake: when to modularize"

---

### Phase 7  Render backend abstraction (weeks 9-10)
1. Define minimal `IGraphicsDevice` + `IGraphicsContext` interfaces.
2. Adapt existing OpenGL code to the interface.
3. Stub Vulkan & Metal back-ends (compilable, returning `NotImplemented`).
4. Compile-time switch via CMake option `FU_GRAPHICS_API`.
5. Runtime factory for future hot-swap on mobile.

**Recommended reading**
• _Game Engine Architecture_ – J. Gregory → Ch. 14 (Rendering Engine)
• *Graphics API Abstraction* – blog series by Arseny Kapoulkine
• Khronos docs – "Portability considerations between Vulkan, Metal, DX12"

---

### Phase 8  Non-Apple platform layer (weeks 10-11)
1. Extract window/input/time into an `IPlatform` interface.
2. Current Win32 implementation → `platform/win`.
3. Add dummy iOS/Android impls to validate abstraction.
4. Eliminate `#ifdef FLEUR_PLATFORM_*` from higher layers.

**Recommended reading**
• SDL2 source – look at `video/` and `events/` back-ends
• *Programming Windows* – (for Win32 specifics)
• Google Codelabs – "NDK Basics" (for Android JNI)

---

### Phase 9  ImGui & Editor bootstrap (weeks 11-12)
1. Wrap ImGui in an engine layer that handles input plumbing & docking.
2. Spawn a dedicated **Editor** executable that links the engine as a DLL.
3. Embed ImGui layer there; game builds still link runtime-only.
4. Expose a minimal C API (plain-old structs) for the upcoming C# host.

**Recommended reading**
• Dear ImGui wiki – "Docking branch & multi-viewport guide"
• *Hands-on Game Development with ImGui* (Packt) → Ch. 4–5
• The Cherno YouTube series – "Hazel Editor" episodes
• Ryan Fleury – "Immediate-mode GUI patterns" (CppNow 2021)

---

### Phase 10  Optimisation & polish (continuous)
* Whole-program LTO & function-level linking.
* Strip RTTI/exceptions in release – keep for editor.
* Asset pipeline: compress textures & meshes offline → zipped asset-pack.
* **Optional**: Embed C# CLI into editor as "File → New Project" wizard.

**Recommended reading**
• Agner Fog – *Optimizing Software in C++*
• *C++ Best Practices* – J. Turner → perf chapters
• Google Perf Docs – "Binaries size reduction"
• Valve's `source-sdk-2013` wiki – "Ship-mode builds & LTO"

---

### Continuous tasks
* Update this roadmap every sprint.
* Keep public headers in `engine/include/` for sub-module users.
* Maintain docs in `docs/` and run Doxygen in CI.
* Grow test-suite beyond CoreLib (Renderer math, allocator edge-cases, event queue).
* **Toolchain evolution**: Python scripts (Phase 0) → C# CLI (Phase 8) → optional editor integration (Phase 9).

---

### Milestones & checkpoints
| Milestone | Planned date | Deliverable |
|-----------|--------------|-------------|
| M1 | end-W2 | Custom smart-pointers removed; PIMPL in Window; build & tests green |
| M2 | end-W4 | C# scripting core integrated; Chip-8 emulator running as C# script |
| M3 | end-W6 | macOS platform layer in Swift; ServiceLocator eliminated; iOS stub working |
| M4 | end-W8 | Policy-based subsystems integrated; OpenGL back-end behind interface; Editor app starts with ImGui |
| M5 | end-Q3 | First mobile demo (iOS + Metal or Android + OpenGL ES) |
| M6 | end-Q4 | C# hot-reload script driving in-engine cube; `Fleur-cli` tool working |

> **NOTE:** timelines are indicative – adjust after each review.
