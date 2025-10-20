#version 300 es
precision mediump float;

in vec3 a_position;
in vec3 a_normal;
in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_lightPos;

out vec4 v_color;
out vec3 v_normal;
out vec3 v_fragPos;
out vec3 v_lightPos;

void main()
{
    v_color = a_color;
    v_normal = mat3(u_model) * a_normal;
    v_fragPos = vec3(u_model * vec4(a_position, 1.0));
    v_lightPos = vec3(u_view * vec4(u_lightPos, 1.0));
    //gl_Position = u_projection * u_view * u_model * vec4(a_position,1.0);
    gl_Position = u_projection * u_view * vec4(v_fragPos,1.0);
}

