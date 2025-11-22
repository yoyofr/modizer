precision mediump float;

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec4 vertex_color;
layout(location = 8) in vec4 vertex_border;

uniform mat4 vertex_transformation;
uniform float vertex_point_size;

out vec4 fragment_color;
out vec4 fragment_border;

void main(){
    gl_Position = vertex_transformation * vec4(vertex_position, 0.0, 1.0);
    gl_Position.z = vertex_border.w;
    gl_PointSize = vertex_point_size;
    fragment_color = vertex_color;
    fragment_border = vertex_border;
}
