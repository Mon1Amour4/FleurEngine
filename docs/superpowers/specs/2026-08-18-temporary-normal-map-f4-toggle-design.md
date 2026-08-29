# Temporary F4 Normal-Mapping Toggle

## Goal

Add a temporary debug switch on `F4` that enables or disables normal-map shading for the opaque Vulkan pass. The change must be easy to remove after visual debugging.

## Design

- Store the toggle in the renderer as a temporary boolean, enabled by default.
- Handle `Key::F4` in `Application::OnKeyPressEvent` and call a renderer toggle method.
- Pass the state to the opaque fragment shader through an existing material/push-constant field, avoiding a descriptor or pipeline-layout change.
- When enabled, sample and decode the normal map as today. When disabled, use the interpolated `worldSpaceNormal`.
- Mark all temporary code with `TEMP_DEBUG_F4_NORMAL_MAP`.

## Verification

- Build the affected target/shaders.
- Confirm F4 changes the rendered lighting between normal-map and geometry-normal paths.
- Confirm the default state remains normal mapping enabled.

## Removal

Remove the marked toggle field, F4 handler, and shader branch together after debugging.
