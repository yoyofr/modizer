#version 300 es
precision mediump float;

in vec4 v_color;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = v_color;
}
