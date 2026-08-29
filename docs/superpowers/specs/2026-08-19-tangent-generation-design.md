# Tangent Generation and Normal-Mapping Basis

## Goal

Make normal mapping use a valid tangent basis for every imported model primitive.
When glTF provides `TANGENT`, the importer preserves it. When it does not, the
importer computes tangents from positions and UVs and stores them in
`SVertexData::Tangent`.

## Scope

- Parse glTF tangent attributes as four floats: xyz tangent direction and w
  handedness, using accessor-aware reads that support byte offsets, interleaved
  buffers, normalized component formats, and sparse accessors.
- Generate fallback tangents per primitive from indexed positions and UVs.
- Handle degenerate UVs without division by zero.
- Generate normals when a primitive has no valid normal attribute.
- Support non-indexed triangle primitives by creating sequential output indices;
  reject unsupported primitive modes with a diagnostic.
- Orthonormalize the final tangent against the vertex normal.
- Store handedness in `Tangent.w` as `+1` or `-1`.
- Transform normals and tangents correctly in the vertex shader.
- Keep shadow passes free of tangent vertex fetches while preserving the full
  `SVertexData` stride.

## Import algorithm

For each primitive, first read positions, normals, UVs, and optional tangents.
If tangents are present and valid, use their direction and handedness as input
to the finalization step. Otherwise, accumulate fallback tangent and bitangent
vectors for each output vertex. Imported tangents are also orthonormalized, so
the final tangent is not required to preserve the source xyz bit-for-bit.

The current engine uses the UV set associated with the normal texture, falling
back to `TEXCOORD_0` when no normal texture is present. If that UV set is not
available, the primitive receives a deterministic fallback tangent and the
normal-map path is disabled for that material.

For a triangle `(p0, p1, p2)` with UVs `(uv0, uv1, uv2)`:

```text
edge1 = p1 - p0
edge2 = p2 - p0
duv1  = uv1 - uv0
duv2  = uv2 - uv0
r     = 1 / (duv1.x * duv2.y - duv1.y * duv2.x)

tangent   = (edge1 * duv2.y - edge2 * duv1.y) * r
bitangent = (edge2 * duv1.x - edge1 * duv2.x) * r
```

If `abs(determinant) <= epsilon`, the triangle contributes no tangent and a
deterministic fallback tangent is generated from its normal later. The absolute
value is required so mirrored UVs with a negative determinant remain valid.

After accumulation:

```text
T = normalize(accumulatedT - N * dot(N, accumulatedT))
w = dot(cross(N, T), accumulatedB) < 0 ? -1 : +1
```

If the accumulated tangent is too small, choose the world axis least aligned
with `N`, then calculate `T = normalize(axis - N * dot(N, axis))`.
If `N` is missing or invalid, generate a normal first; if a valid normal still
cannot be produced, reject the primitive rather than emitting NaNs. The tangent
finalization API must expose this failure to the importer instead of silently
writing a default tangent.

Fallback tangents use `Tangent.w = +1`. Vertices are split when required to
avoid interpolating incompatible tangent handedness across a triangle seam.

## Shader basis

For `world = model * node`, the vertex shader transforms the normal using
`transpose(inverse(mat3(world)))` and transforms the tangent using
`mat3(world)`. The fragment shader re-orthogonalizes the interpolated tangent
against the interpolated normal and reconstructs:

```glsl
B = normalize(cross(N, T)) * effectiveHandedness;
```

When `det(mat3(world)) < 0`, `effectiveHandedness` negates the imported
handedness. This preserves the tangent basis under mirrored world transforms.

The tangent-space normal is decoded from `[0, 1]` to `[-1, 1]` and transformed
with `mat3(T, B, N)` when the temporary F4 normal-mapping toggle is enabled.

## Vertex layout and passes

`SVertexData` remains the source of truth for the model buffer. Its layout is
declared explicitly with compile-time checks for size, offsets, formats, and
full stride; it is not inferred from the number of active shader inputs. Opaque
uses the tangent attribute. Shadow passes do not declare or fetch tangent, but
their pipeline layout uses the full `sizeof(SVertexData)` stride and explicit
offsets for the attributes they consume.

The normal-map shader branch executes only when both F4 is enabled and the
material has a valid normal texture. Texture indices must never use `-1` as an
unsigned shader index; materials without a normal map use a safe fallback index
and a false availability flag.

## Validation

- A model with glTF tangents preserves the imported handedness and produces a
  finite tangent orthogonal to the normal.
- Interleaved, offset, normalized, and sparse accessors are read correctly.
- A model without glTF tangents receives finite, normalized tangents.
- A model without normals receives generated finite normals or is rejected
  without producing NaN.
- Generated tangent is orthogonal to the normal within tolerance.
- Mirrored UVs produce the correct handedness sign.
- Degenerate UV triangles do not produce NaN or infinity.
- F4 visibly switches between geometric and normal-mapped lighting.
- A material without a normal map never samples an invalid texture index.
- Shadow passes retain the 48-byte model stride without fetching tangent.

## Non-goals

- Replacing the generator with a full MikkTSpace implementation in this change.
- Changing point-light attenuation or the specular BRDF.
- Removing the temporary F4 toggle.
