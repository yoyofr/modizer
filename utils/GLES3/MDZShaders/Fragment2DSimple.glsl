#version 300 es
precision mediump float;

in vec4 v_color;
out vec4 outColor;

uniform int u_mode;

void main()
{
    outColor = v_color;
//    outColor = vec4(v_color.rgb*v_color.rgb,v_color.a);
}
