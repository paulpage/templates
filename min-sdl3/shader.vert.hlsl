struct VertexData {
    float4 rect;
    float4 color;
};

struct Output {
    float4 rect : TEXCOORD0;
    float4 color : TEXCOORD1;
    float4 position : SV_Position;
};

ByteAddressBuffer data : register(t0, space0);

cbuffer UniformBlock : register(b0, space1) {
    float2 screen_size : packoffset(c0);
};

static const uint tri_idx[6] = {0, 1, 2, 2, 3, 0};

VertexData LoadVertex(uint idx) {
    VertexData v;
    uint offset = idx * 32;
    v.rect  = asfloat(data.Load4(offset +  0));
    v.color = asfloat(data.Load4(offset + 16));
    return v;
}

Output main(uint id : SV_VertexID) {
    VertexData d = LoadVertex(id / 6);
    uint p = id % 6;

    float2 vert_pos[4] = {
        float2(d.rect.x, d.rect.y),
        float2(d.rect.x, d.rect.y + d.rect.w),
        float2(d.rect.x + d.rect.z, d.rect.y + d.rect.w),
        float2(d.rect.x + d.rect.z, d.rect.y),
    };

    Output output;
    output.rect = d.rect;
    output.color = d.color;
    output.position = float4(
        (vert_pos[tri_idx[p]] / (screen_size / 2) - 1) * float2(1, -1),
        0, 1
    );
    return output;
}
