#include <metal_stdlib>
using namespace metal;

struct VertexIn
{
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
};

struct VertexOut
{
    float4 position [[position]];
    float3 fragColor;
};

vertex VertexOut vs_triangle(VertexIn in [[stage_in]])
{
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.fragColor = in.color;
    return out;
}

fragment float4 ps_triangle(VertexOut in [[stage_in]])
{
    return float4(in.fragColor, 1.0);
}
