#version 300 es
precision mediump float;

in vec2 v_textCoord;

out vec4 outColor;

uniform sampler2D u_curTexture;
uniform int u_horizontal;
uniform float u_min_brightness;
uniform float u_blurDivider;
float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
float boost_weight=1.0;
float alpha_thres=0.01;

void main()
{
    vec2 tex_offset = vec2(1.0f,1.0f) / vec2(textureSize(u_curTexture, 0)); // gets size of single texel
//    vec3 result = texture(u_curTexture, v_textCoord).rgb * weight[0]; // current fragment's contribution
//    if(u_horizontal==1)
//    {
//        for(int i = 1; i < 5; ++i)
//        {
//            float i_float=float(i);
//            result += texture(u_curTexture, v_textCoord + vec2(tex_offset.x * i_float, 0.0)).rgb * weight[i]*boost_weight;
//            result += texture(u_curTexture, v_textCoord - vec2(tex_offset.x * i_float, 0.0)).rgb * weight[i]*boost_weight;
//        }
//    }
//    else
//    {
//        for(int i = 1; i < 5; ++i)
//        {
//            float i_float=float(i);
//            result += texture(u_curTexture, v_textCoord + vec2(0.0, tex_offset.y * i_float)).rgb * weight[i]*boost_weight;
//            result += texture(u_curTexture, v_textCoord - vec2(0.0, tex_offset.y * i_float)).rgb * weight[i]*boost_weight;
//        }
//    }
    vec2 stepsize=tex_offset*float(u_horizontal);
    vec4 result = vec4(0);
    float brightness=length(texture(u_curTexture,v_textCoord).rgb);
    if (brightness>=u_min_brightness) {
        for(int x = -1; x<=1; x++)
            for(int y = -1; y<=1; y++)
            {
                float x_float=float(x);
                float y_float=float(y);
                result += texture(u_curTexture, v_textCoord + vec2(x_float,y_float)*stepsize);
            }
        result += texture(u_curTexture, v_textCoord);
        result /= u_blurDivider;
        
//         outColor = vec4(result.rgb,1.0);
        //    outColor = result;
        float alpha=length(result.rgb);
        //if (alpha>alpha_thres) alpha=1.0;
        outColor = vec4(result.rgb, alpha);
    } else outColor = vec4(0,0,0,0);
}

