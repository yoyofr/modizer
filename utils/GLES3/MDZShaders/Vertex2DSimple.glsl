#version 300 es
precision mediump float;
uniform mat4 u_mvpMatrix;
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec4 a_color;

out vec4 v_color;

void main()
{
    v_color = a_color;
    gl_Position = u_mvpMatrix*vec4(a_position.xy,0,1);
}

