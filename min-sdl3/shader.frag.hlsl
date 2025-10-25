struct Input {
    float4 rect : TEXCOORD0;
    float4 color : TEXCOORD1;
    float4 position : SV_Position;
};

cbuffer UniformBlock : register(b0, space3) {
    float2 screen_size : packoffset(c0);
};

float4 main(Input input) : SV_Target0 {

    // This works with shadercross and dxc:
    float4 color = input.color;
    color.r += (input.rect.x * input.rect.y * input.rect.z * input.rect.w * input.position.x * input.position.y * input.position.z * input.position.w * 0.000001);
    return color;

    // This does not work with shadercross:
    // return input.color;
}
