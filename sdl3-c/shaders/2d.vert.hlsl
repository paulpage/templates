struct VertexData {
    float4 dst_rect;
    float4 src_rect;
    float4 colors[4];
    float4 corner_radii;
    float edge_softness;
    float border_thickness;
};

struct Output {
    float2 tex_coord : TEXCOORD0;
    float4 color : TEXCOORD1;
    float4 position : SV_Position;
};

StructuredBuffer<VertexData> data : register(t0, space0);

cbuffer UniformBlock : register(b0, space1) {
    float2 screen_size : packoffset(c0);
};

Output main(uint id : SV_VertexID) {
    Output output;
    output.tex_coord = float2(0, 0);
    output.color = float4(0, 0, 0, 0);
    output.position = float4(0, 0, 0, 0);
    return output;
}
