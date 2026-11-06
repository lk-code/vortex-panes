// Instanced snowflakes: one instance per flake, four vertices generated from SV_VertexID
// (triangle strip), no vertex buffer. Positions are given in virtual-screen pixels and mapped
// to the monitor's normalized device coordinates. Shader model 4.0 (feature level 10_0 and WARP).

cbuffer FrameConstants : register(b0)
{
    float2 originPx;  // monitor left/top in virtual-screen pixels
    float2 invSizePx; // 1 / monitor width, 1 / monitor height
    float opacity;    // global fade factor
};

struct Instance
{
    float4 flake : FLAKE; // center x, center y (px), radius (px), alpha
};

struct VsOut
{
    float4 position : SV_Position;
    float2 offsetPx : TEXCOORD0;               // offset from the flake center in pixels
    nointerpolation float2 edgePx : TEXCOORD1; // distances where the soft edge starts and ends
    nointerpolation float alpha : TEXCOORD2;
};

// Width of the anti-aliased edge; matches kFlakeEdgePx and flakeExtent() in src/core/flake.h.
static const float kEdgePx = 1.0;

VsOut main(Instance instance, uint vertexId : SV_VertexID)
{
    const float radius = instance.flake.z;
    // Flakes below one pixel are drawn with a radius of one pixel and an alpha reduced by the
    // area ratio. Together with an edge at least one pixel wide, this keeps small flakes from
    // flickering when they move by fractions of a pixel.
    const float drawRadius = max(radius, 1.0);
    const float areaScale = min(radius * radius, 1.0);
    const float extent = drawRadius + kEdgePx;

    const float2 corner = float2(vertexId & 1, vertexId >> 1) * 2.0 - 1.0; // (-1,-1) (1,-1) (-1,1) (1,1)
    const float2 offset = corner * extent;
    const float2 px = instance.flake.xy + offset;

    VsOut output;
    output.position = float4((px.x - originPx.x) * invSizePx.x * 2.0 - 1.0,
                             1.0 - (px.y - originPx.y) * invSizePx.y * 2.0, 0.0, 1.0);
    output.offsetPx = offset;
    // Opaque core up to 60 % of the radius, then a falloff ending half a pixel outside it.
    output.edgePx = float2(max(drawRadius * 0.6 - 0.5, 0.0), drawRadius + 0.5);
    output.alpha = instance.flake.w * areaScale * opacity;
    return output;
}
