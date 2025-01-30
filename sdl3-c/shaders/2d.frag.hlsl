Texture2D<float4> texture : register(t0, space2);
SamplerState sam : register(s0, space2);

struct Input {
    float2 tex_coord : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(Input input) : SV_Target0 {
    return input.color * texture.Sample(sam, input.tex_coord);
    // return input.color;
}
