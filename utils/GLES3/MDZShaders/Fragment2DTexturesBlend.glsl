#version 300 es
precision mediump float;

in vec2 v_textCoord;

out vec4 outColor;

uniform sampler2D u_textOriginal;
uniform sampler2D u_textBlurred;
uniform float u_alpha;
uniform float u_exposure;

void main()
{
    const float gamma = 2.2;
    vec3 hdrColor = texture(u_textOriginal, v_textCoord).rgb;
    vec3 bloomColor = texture(u_textBlurred, v_textCoord).rgb;
    float alphaA=texture(u_textOriginal, v_textCoord).a;
    float alphaB=texture(u_textBlurred, v_textCoord).a;
    float alpha=alphaB;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * u_exposure);
    // also gamma correct while we're at it
    result = pow(result, vec3(1.0 / gamma));
    outColor = vec4(result, alpha);
}

