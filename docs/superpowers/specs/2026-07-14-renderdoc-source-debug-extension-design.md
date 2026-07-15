# RenderDoc Source Shader Debugger Extension

## Goal

Create a RenderDoc extension for Vulkan shader debugging that presents the original GLSL source together with values of source-level variables, while hiding compiler-generated SPIR-V temporaries.

The primary workflow is debugging a selected fragment/pixel invocation in a RenderDoc capture.

## User experience

The extension provides a dockable tool window with:

- the original GLSL source, syntax-highlighted and scrolled to the current debug line;
- an `All variables` view for visible source variables at the current point;
- a `Watch` view for pinned variables and expressions;
- values, types, source line, scope, and availability status;
- a link from a source variable to its underlying RenderDoc debug instruction(s);
- capture, event, shader stage, entry point, and pixel context in the header.

For `opaqueFragment.frag`, the preferred visible names include `V`, `L`, `I`, `albedo`, `NdotL`, `NdotH`, `ambient`, `diffuse`, `specular`, `pointLightColor`, `shadow`, and `outColor`. SSA names such as `_185`, `_186`, `_187`, pointer placeholders, and compiler-only temporaries are hidden by default.

The extension supports two complementary value modes:

1. current-line values for variables available at the selected source line;
2. watch values for explicitly pinned variables or supported expressions.

Unavailable values are displayed as `unavailable`; the extension must not substitute an unrelated compiler temporary.

## Scope

### In scope for MVP

- Vulkan fragment/pixel shader debugging;
- automatic source-file resolution;
- selected pixel/event context from the active RenderDoc capture;
- original GLSL source view;
- source variable normalization and filtering;
- current-line values and watch values;
- basic search and variable pinning;
- clear diagnostics when debug information or a value mapping is missing.

### Out of scope for MVP

- replacing RenderDoc's built-in shader debugger;
- CPU reimplementation of shader execution;
- automatic rewriting of shader source;
- full expression-language support;
- vertex, compute, geometry, and ray-tracing stages;
- editing and recompiling shaders from the extension;
- a separate standalone debugger application.

## Recommended architecture

Use a Python RenderDoc extension for the initial implementation. Keep the user-facing model independent from RenderDoc's raw API objects so that a native bridge can be introduced later without changing the UI.

```text
Active RenderDoc capture
        |
        v
Selected event + pixel context
        |
        v
RenderDoc debug trace / shader reflection
        |
        v
Source resolver + SPIR-V/source mapper
        |
        v
Normalized source variable model
        |
        v
GLSL source view + values + Watch panel
```

### Extension integration layer

Responsible for:

- registering the extension panel and menu action;
- detecting the active capture and selected event/pixel context;
- invoking the RenderDoc replay/debug operation;
- refreshing the panel after capture or selection changes;
- reporting API/version incompatibilities.

### Source resolver

Resolve the source automatically using this order:

1. source path from shader debug information;
2. source embedded in the capture;
3. shader name and entry point metadata;
4. configured source roots;
5. a local shader-hash index built by the extension.

The resolver records the selected path, shader hash, entry point, and source freshness status. If the file is unavailable, the extension uses embedded source when possible and otherwise shows a diagnostic with the paths it searched.

### Source mapper

Build a mapping between source variables/lines and RenderDoc debug values using:

- source line metadata when present;
- SPIR-V debug metadata and source names;
- reflection data for inputs, outputs, push constants, and resources;
- conservative name and scope matching for optimized variables.

The mapper must distinguish exact, inferred, and unavailable mappings. Inferred mappings are visually marked and never silently presented as exact.

### Normalized variable model

Each displayed value contains:

- source name;
- type;
- formatted value;
- source line;
- scope;
- current value state;
- mapping confidence;
- underlying SPIR-V/debug identifiers;
- pinned/watch state;
- child values for structs and vectors where available.

The UI consumes this model and never reads raw RenderDoc debugger objects directly.

### Filtering

Hide by default:

- SSA names such as `_185`;
- pointer placeholders such as `float4*`;
- unnamed compiler temporaries;
- duplicate values with no source-level identity;
- internal control-flow bookkeeping.

Always preserve:

- user-named source variables;
- function parameters and shader inputs/outputs;
- push constants and resource bindings;
- pinned variables;
- variables needed to explain an explicitly selected source expression.

Provide a diagnostic toggle to reveal raw values when the curated mapping is insufficient.

## Refresh and data flow

1. The user opens a capture and selects an event/pixel.
2. The extension reads the active shader stage and entry point.
3. The source resolver locates the GLSL source.
4. The extension requests or consumes the RenderDoc debug trace.
5. The mapper associates trace values with source lines and names.
6. The filter removes compiler-only values.
7. The UI updates the source cursor, variable table, and Watch panel.
8. The extension caches results by capture, event, pixel, shader hash, and debug mode.

Refreshes must be asynchronous where RenderDoc permits it, and stale results must not replace a newer selection.

## Error handling

- No capture: show an empty state with instructions.
- No selected pixel/event: show the source and request a selection.
- Source not found: show embedded source or searched paths.
- Debug info unavailable: show source with a reduced mapping-confidence notice.
- Value unavailable: show `unavailable` and preserve the source variable.
- Unsupported stage/API/version: show a non-fatal compatibility message.
- Debug trace failure: keep reflection and source data visible.
- Large trace: limit visible values, use lazy expansion, and keep the UI responsive.

## Testing

### Unit tests

- source resolver priority and fallback behavior;
- shader hash and entry-point matching;
- filtering of SSA/pointer/compiler temporaries;
- preservation of user variables and pinned watches;
- exact/inferred/unavailable mapping states;
- value formatting for scalars, vectors, matrices, structs, and arrays.

### Integration tests

- load a Vulkan capture and resolve `opaqueFragment.frag` automatically;
- refresh when event or pixel selection changes;
- show `albedo`, `NdotL`, `diffuse`, and `outColor` without `_185`-style noise;
- preserve a Watch list across selection changes within the same capture;
- invalidate cache when the capture or shader hash changes;
- display a useful diagnostic for the current `Source debugging Unavailable` condition.

### Manual acceptance test

For the supplied opaque fragment shader, select a pixel affected by the draw and verify that the extension lets the user inspect the original source and current/watch values without manually reading the RenderDoc disassembly.

## Risks and fallback

The main technical risk is whether the installed RenderDoc Python API exposes enough of the shader-debug trace to reconstruct current-line values. The MVP therefore isolates the RenderDoc adapter. If Python cannot access the required states, keep the same normalized model and UI, then replace only the adapter with a small native bridge or a narrowly scoped RenderDoc patch.

The extension must not emulate shader execution or claim exactness when the trace cannot support it.

## Milestones

1. Extension panel scaffold and source resolver.
2. Source view, normalized model, and temporary filtering using supplied shader/SPIR-V fixtures.
3. RenderDoc selection/debug adapter and current-line values.
4. Watch values, diagnostics, caching, and manual validation against a Vulkan capture.

