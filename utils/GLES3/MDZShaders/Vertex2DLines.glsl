#version 300 es
precision mediump float;
uniform mat4 u_mvpMatrix;
uniform float u_width;
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec4 a_pointAB;
layout (location = 2) in vec4 a_color;

out vec2 v_uv;
out vec4 v_color;

void main() {
    v_color = a_color;
    v_uv=a_position;
    
    // A-B line vector
    vec2 pA=a_pointAB.xy;
    vec2 pB=a_pointAB.zw;
    //pA = vec2(0.0,0.0);
    //pB = vec2(1.0,0.0);
    vec2 xBasis = pB - pA;
    // perpendicular vector / A-B line
    vec2 yBasis = normalize(vec2(-xBasis.y, xBasis.x));
    // compute new position
    float width = u_width;
    vec2 point = pA + xBasis * a_position.x + yBasis * width * a_position.y;
    // apply projection
    gl_Position = u_mvpMatrix*vec4(point.xy,0.0f,1.0f);
}
