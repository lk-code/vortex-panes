// Fills a rectangle or a soft circle. The color is premultiplied, so scaling all four
// channels by the coverage keeps it premultiplied (blend: ONE, INV_SRC_ALPHA).

struct PsIn
{
    float4 position : SV_Position;
    float2 local : TEXCOORD0;
    float4 color : COLOR;
    nointerpolation float shape : SHAPE;
};

float4 main(PsIn input) : SV_Target
{
    float coverage = 1.0;
    if (input.shape > 0.5)
    {
        const float distance = length(input.local);
        coverage = 1.0 - smoothstep(0.6, 1.0, distance);
    }
    return input.color * coverage;
}
