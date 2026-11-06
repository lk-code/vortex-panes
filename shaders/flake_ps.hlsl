// Soft, anti-aliased white disc. The output is premultiplied (blend: ONE, INV_SRC_ALPHA).

struct PsIn
{
    float4 position : SV_Position;
    float2 offsetPx : TEXCOORD0;
    nointerpolation float2 edgePx : TEXCOORD1;
    nointerpolation float alpha : TEXCOORD2;
};

float4 main(PsIn input) : SV_Target
{
    const float coverage = 1.0 - smoothstep(input.edgePx.x, input.edgePx.y, length(input.offsetPx));
    const float a = input.alpha * coverage;
    return float4(a, a, a, a);
}
