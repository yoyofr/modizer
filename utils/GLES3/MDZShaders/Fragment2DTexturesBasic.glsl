#version 300 es
precision mediump float;

in vec2 v_textCoord;

out vec4 outColor;

uniform sampler2D u_curTexture;
uniform float u_alpha;

void main()
{
    vec2 tc=v_textCoord;
    if (tc.x>1.0f) tc.x=1.0f;
    if (tc.y>1.0f) tc.y=1.0f;
    if (tc.x<0.0f) tc.x=0.0f;
    if (tc.y<0.0f) tc.y=0.0f;
    vec4 pixel = texture(u_curTexture,tc);
    outColor = vec4(pixel.xyz,u_alpha);
}
