#version 300 es
precision mediump float;

in vec2 v_textCoord;

out vec4 outColor;

uniform sampler2D u_curTexture;
uniform int u_horizontal;
uniform float u_min_brightness;
float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
float boost_weight=1.0;
float alpha_thres=0.02;

void main()
{
    vec2 tex_offset = vec2(1.0f,1.0f) / vec2(textureSize(u_curTexture, 0)); // gets size of single texel
    vec3 result = texture(u_curTexture, v_textCoord).rgb * weight[0]; // current fragment's contribution
    if(u_horizontal==1)
    {
        for(int i = 1; i < 5; ++i)
        {
            float i_float=float(i);
            result += texture(u_curTexture, v_textCoord + vec2(tex_offset.x * i_float, 0.0)).rgb * weight[i]*boost_weight;
            result += texture(u_curTexture, v_textCoord - vec2(tex_offset.x * i_float, 0.0)).rgb * weight[i]*boost_weight;
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            float i_float=float(i);
            result += texture(u_curTexture, v_textCoord + vec2(0.0, tex_offset.y * i_float)).rgb * weight[i]*boost_weight;
            result += texture(u_curTexture, v_textCoord - vec2(0.0, tex_offset.y * i_float)).rgb * weight[i]*boost_weight;
        }
    }
    float alpha=length(result);
    if (alpha>alpha_thres) alpha=1.0;
//    alpha=1.0;
    outColor = vec4(result, alpha);
}

