# RenderDoc Extension Shader Viewer Design

## Goal
Build a RenderDoc extension that automatically inspects the currently selected capture/draw call and presents shader-relevant data in a clean, human-readable UI.

The tool must not require manual copy/paste of shader dumps or RenderDoc output. It should pull data from the active RenderDoc capture and render only the values that matter.

## Problem
Current shader debugging UIs expose too many temporary variables and low-level implementation details. That makes it harder to understand what actually influences the final output.

The desired experience is:
- open a capture in RenderDoc
- select a draw call or pixel
- the extension updates automatically
- only curated values are shown

## Scope
### In scope
- RenderDoc UI extension
- automatic reading of active capture state
- draw call selection awareness
- shader stage metadata display
- curated display of inputs, outputs, uniforms, and selected intermediates
- filtering/hiding of compiler-generated temporary values by default

### Out of scope for MVP
- standalone desktop app
- manual paste/import workflow
- full replacement for RenderDoc frame debugger
- CPU shader emulation
- full disassembly viewer

## Recommended Approach
Use a RenderDoc extension rather than a separate program.

Reasons:
- zero manual data transfer
- data is already available in the host app
- the UI can be shown directly where the user already works
- less friction than exporting/importing captures

## Product Shape
The extension should expose one main tool window with three areas:

1. Capture context
- active capture name
- current event / draw call
- pipeline stage selection

2. Clean value tree
- user-facing inputs
- outputs
- uniforms / push constants
- selected texture bindings
- user-marked watch values

3. Detail inspector
- per-value type
- source field or semantic name
- current value
- optional raw/debug metadata

## Data Flow
1. User opens a capture in RenderDoc.
2. RenderDoc notifies the extension about the current capture/event context.
3. The extension reads the selected draw call and associated shader/pipeline data.
4. The extension filters out compiler-generated noise.
5. The extension builds a curated view model.
6. The UI renders the view model and refreshes when selection changes.

## Filtering Rules
Default rules should hide:
- compiler temporaries
- implementation-only SSA-style names
- unnamed intermediates
- repeated redundant values

Default rules should show:
- user-declared variables
- uniforms
- push constants
- texture/sampler bindings
- shader stage inputs and outputs
- explicitly pinned watch values

## Architecture
### Extension layer
Responsible for integration with RenderDoc, capture/event notifications, and access to selected frame data.

### Extraction layer
Responsible for converting RenderDoc data into a normalized internal model.

### Filtering layer
Responsible for suppressing compiler noise and applying naming/visibility rules.

### UI layer
Responsible for rendering the clean tree, search/filter controls, and selection details.

## Error Handling
The extension should fail softly:
- if capture data is missing, show an empty state
- if a shader stage cannot be resolved, show a stage-specific warning
- if some values are unavailable, keep the rest of the tree visible
- if parsing or extraction fails, show the error in the panel rather than crashing RenderDoc

## Testing
### Functional checks
- capture loads and the panel updates automatically
- draw call change updates the data view
- compiler temporaries are hidden by default
- user-defined variables remain visible
- missing data produces a readable empty/error state

### Regression checks
- multiple captures can be opened without stale state leaking between them
- switching between vertex and fragment stages updates the tree correctly
- large debug outputs do not freeze the UI

## MVP Milestone
The first milestone should support:
- active capture detection
- selected draw call inspection
- structured tree output
- default filtering of temporaries
- basic search/filter text box

That is enough to validate whether the curated view is actually easier to use than the raw debugger tree.

## Future Extensions
After MVP, the same model can be extended with:
- custom watch expressions
- per-variable pinning
- stage-to-stage value tracing
- side-by-side comparisons between two captures
- optional export for reports or bug tickets

