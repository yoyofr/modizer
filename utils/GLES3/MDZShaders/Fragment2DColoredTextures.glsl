#version 300 es
precision mediump float;

in vec2 v_textCoord;
in vec4 v_color;

out vec4 outColor;

uniform sampler2D u_curTexture;

void main()
{
    vec2 tc=v_textCoord;
    if (tc.x>1.0f) tc.x=1.0f;
    if (tc.y>1.0f) tc.y=1.0f;
    if (tc.x<0.0f) tc.x=0.0f;
    if (tc.y<0.0f) tc.y=0.0f;
    vec4 pixel = texture(u_curTexture,tc)*v_color;
    outColor = pixel;
}
