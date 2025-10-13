#version 300 es
precision mediump float;

in vec2 v_textCoord;

out vec4 outColor;

uniform sampler2D u_curTexture;
uniform float u_alpha;
uniform int u_horizontal;
float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = vec2(1.0f,1.0f) / vec2(textureSize(u_curTexture, 0)); // gets size of single texel
    vec3 result = texture(u_curTexture, v_textCoord).rgb * weight[0]; // current fragment's contribution
    if(u_horizontal==1)
    {
        for(int i = 1; i < 5; ++i)
        {
            float i_float=float(i);
            result += texture(u_curTexture, v_textCoord + vec2(tex_offset.x * i_float, 0.0)).rgb * weight[i];
            result += texture(u_curTexture, v_textCoord - vec2(tex_offset.x * i_float, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            float i_float=float(i);
            result += texture(u_curTexture, v_textCoord + vec2(0.0, tex_offset.y * i_float)).rgb * weight[i];
            result += texture(u_curTexture, v_textCoord - vec2(0.0, tex_offset.y * i_float)).rgb * weight[i];
        }
    }
    float result_len=length(result);
    if (result_len>0.0) outColor = vec4(result, 1.0);
    else outColor = vec4(result, 0.0);
}

