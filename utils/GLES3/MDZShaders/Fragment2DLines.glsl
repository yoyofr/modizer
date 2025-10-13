#version 300 es
precision mediump float;

in vec4 v_color;
out vec4 outColor;

in vec2 v_uv;

void main()
{
    float dist=v_uv.y*v_uv.y;
    dist=(0.25f-dist)*6.0f;
    if (dist>1.0f) dist=1.0f;
    if (dist<0.0f) dist=0.0f;
    outColor = v_color*pow(dist,3.0);
}
