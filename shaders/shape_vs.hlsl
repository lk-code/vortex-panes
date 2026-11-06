// Instanced quad: one instance per shape, four vertices generated from SV_VertexID
// (triangle strip). Positions are given in virtual-screen pixels and mapped to the
// monitor's normalized device coordinates. Shader model 4.0 (feature level 10_0 and WARP).

cbuffer FrameConstants : register(b0)
{
    float2 originPx;  // monitor left/top in virtual-screen pixels
    float2 invSizePx; // 1 / monitor width, 1 / monitor height
};

struct Instance
{
    float4 rectPx : RECT;   // left, top, right, bottom in virtual-screen pixels
    float4 color : COLOR;   // premultiplied RGBA
    float shape : SHAPE;    // 0 = rectangle, 1 = soft circle
};

struct VsOut
{
    float4 position : SV_Position;
    float2 local : TEXCOORD0;           // -1..1 across the quad
    float4 color : COLOR;
    nointerpolation float shape : SHAPE;
};

VsOut main(Instance instance, uint vertexId : SV_VertexID)
{
    const float2 corner = float2(vertexId & 1, vertexId >> 1); // (0,0) (1,0) (0,1) (1,1)
    const float2 px = lerp(instance.rectPx.xy, instance.rectPx.zw, corner);

    VsOut output;
    output.position = float4((px.x - originPx.x) * invSizePx.x * 2.0 - 1.0,
                             1.0 - (px.y - originPx.y) * invSizePx.y * 2.0, 0.0, 1.0);
    output.local = corner * 2.0 - 1.0;
    output.color = instance.color;
    output.shape = instance.shape;
    return output;
}
