#version 300 es
precision mediump float;
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 2) in vec4 a_textCoord;

out vec2 v_textCoord;
out vec4 v_color;

void main()
{
    gl_Position = vec4(a_position.xy,0.0f,1.0f);
    v_color = a_color;
    v_textCoord = vec2(a_textCoord.x,a_textCoord.y);
}
