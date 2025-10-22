#version 300 es
precision mediump float;

in vec4 v_color;
layout(location = 0) out vec4 outColor;

void main()
{
    int x=int(gl_FragCoord.y);
    if ((x&15)<10) outColor = v_color;
    else outColor = vec4(0,0,0,0);
}
