#version 300 es
precision mediump float;

in vec4 v_color;
layout(location = 0) out vec4 outColor;

uniform int u_mode;

void main()
{
    int x=int(gl_FragCoord.y);
    if (u_mode==0) {
        if ((x&15)<10) outColor = v_color;
        else outColor = vec4(0,0,0,0);
    } else if (u_mode==1) {
        vec4 col;
        if (x>(256-96)) col=vec4(0.9,0.2,0.14,v_color.a);
        else col=v_color;
        if ((x&15)<10) outColor = col;
        else outColor = vec4(0,0,0,0);
    } else outColor = vec4(0,0,0,0);
}
