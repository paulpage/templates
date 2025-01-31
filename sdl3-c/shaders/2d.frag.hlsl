Texture2D<float4> texture : register(t0, space2);
SamplerState sam : register(s0, space2);

struct Input {
    float2 tex_coord : TEXCOORD0;
    float4 rect : RECT;
    float4 color : TEXCOORD1;
    float4 corner_radii : RADII;
    float4 position : SV_Position; // clip space!
};

cbuffer UniformBlock : register(b0, space3) {
    float2 screen_size : packoffset(c0);
};

float sdf_rounded_box(float2 p, float2 b, float4 r) {
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    float2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

float4 main(Input input) : SV_Target0 {
    // return input.color * texture.Sample(sam, input.tex_coord);

    float2 half_size = 2 * input.rect.zw / screen_size.y / 2;
    float2 p = (2 * input.position.xy - screen_size.xy) / screen_size.y - (2 * (input.rect.xy + input.rect.zw / 2) - screen_size.xy) / screen_size.y;
    float d = sdf_rounded_box(p, half_size, input.corner_radii / (screen_size.y / 2));

    return float4(input.color.xyz, 1-smoothstep(0, 0.002, d) * input.color.a);
}
