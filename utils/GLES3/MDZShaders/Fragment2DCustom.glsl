#version 300 es
precision mediump float;

in vec4 v_color;
layout(location = 0) out vec4 outColor;

uniform int u_mode;
uniform int u_redHeight;
uniform vec3 u_redCol;

void main()
{
    int x=int(gl_FragCoord.y);
    if (u_mode==0) {
        if ((x&15)<10) outColor = v_color;
        else outColor = vec4(0,0,0,0);
    } else if (u_mode==1) {
        vec4 col;
        if (x>u_redHeight) col=vec4(u_redCol.rgb,v_color.a);
        else col=v_color;
        if ((x&15)<10) outColor = col;
        else outColor = vec4(0,0,0,0);
    } else outColor = vec4(0,0,0,0);
}
