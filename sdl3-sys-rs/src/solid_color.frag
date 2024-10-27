#version 450

layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 rectMin;
layout (location = 3) in vec2 rectMax;

layout (location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform ScreenSize {
    vec2 uScreenSize;
};

float box(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    //vec2 p = (gl_FragCoord.xy / uScreenSize) * 2.0 - 1.0;
    vec2 rectSize = rectMax - rectMin;
    vec2 halfSize = rectSize / 2.0;
    vec2 center = (rectMin + rectMax) / 2.0;

    vec2 p = -center + (2.0 * gl_FragCoord.xy - uScreenSize) / uScreenSize.y;

    //p.y = -p.y;
    float radius = 0.1;

    float aspectRatio = uScreenSize.x / uScreenSize.y;
    halfSize.y /= aspectRatio;
    radius /= aspectRatio;

    float box = box(p, halfSize, radius);
    float alpha = 1. - smoothstep(0.0, 0.003, box);
    fragColor = vec4(color.rgb, alpha);
}
