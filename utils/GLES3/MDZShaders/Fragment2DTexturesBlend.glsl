#version 300 es
precision mediump float;

in vec2 v_textCoord;

out vec4 outColor;

uniform sampler2D u_textOriginal;
uniform sampler2D u_textBlurred;
//uniform float u_exposure;
float u_exposure=1.0;

void main()
{
    const float gamma = 2.2;
    vec3 hdrColor = texture(u_textOriginal, v_textCoord).rgb;
    vec3 bloomColor = texture(u_textBlurred, v_textCoord).rgb;
    float alpha=texture(u_textBlurred, v_textCoord).a+texture(u_textOriginal, v_textCoord).a;
    if (alpha>1.0) alpha=1.0;
//    if (alpha>0.0)  alpha=1.0;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result;
    
//    result = hdrColor;
    result= vec3(1.0) - exp(-hdrColor * u_exposure);
    // also gamma correct while we're at it
    result = pow(result, vec3(1.0 / gamma));
//    alpha=length(result);
    outColor = vec4(result, alpha);
}

