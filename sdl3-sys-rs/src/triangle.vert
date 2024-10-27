#version 450

layout (location = 0) in vec2 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 rectMin;
layout (location = 3) in vec2 rectMax;

layout (location = 0) out vec2 outPos;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec2 outRectMin;
layout (location = 3) out vec2 outRectMax;

layout(set = 1, binding = 0) uniform ScreenSize {
    vec2 uScreenSize;
};

void main()
{
    //outPos = position;
    outPos = (position / uScreenSize) * 2.0 - 1.0;
    outPos.y = -outPos.y;
    outColor = color / 255.0;
    outRectMin = (rectMin / uScreenSize) * 2.0 - 1.0;
    outRectMax = (rectMax / uScreenSize) * 2.0 - 1.0;
	gl_Position = vec4(outPos, 0, 1);
}
