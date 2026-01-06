#version 300 es
precision mediump float;

in vec4 v_color;
out vec4 out_color;

uniform int u_mode;
uniform vec4 u_resolution;
uniform float u_checkboardsize;
uniform float u_time;
uniform float u_scaleFactor;

vec3 hsv2rgb(vec3 c) {
    vec3 rgb = clamp(
        abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0,
        0.0,
        1.0
    );
    return c.z * mix(vec3(1.0), rgb, c.y);
}

const float BORDER = 2.0;
const float NEON_RADIUS = 20.0;   // portée de la lumière
const float NEON_POWER  = 2.5;    // dureté du glow

void main() {
    float px=gl_FragCoord.x;
    float py=gl_FragCoord.y;
    float incr=u_time*80.0;
    switch (u_mode) {
        case 0://full
            break;
        case 1://left
            px+=incr;
            break;
        case 2://right
            px-=incr;
            break;
        case 3://bottom
            py+=incr;
            break;
        case 4://top
            py-=incr;
            break;
        case 5://bottom left
            px+=incr;
            py+=incr;
            break;
        case 6://bottom right
            px-=incr;
            py+=incr;
            break;
        case 7://top left
            px+=incr;
            py-=incr;
            break;
        case 8://top right
            px-=incr;
            py-=incr;
            break;
    }
    bool a = mod(px, u_checkboardsize) > (u_checkboardsize/2.0);
    bool b = mod(py, u_checkboardsize) > (u_checkboardsize/2.0);

    float x1 = u_resolution.x;
    float y1 = u_resolution.y;
    float x2 = u_resolution.z;
    float y2 = u_resolution.w;
    float w=x2-x1;
    float h=y2-y1;

    // Distance aux bords
    float dLeft   = gl_FragCoord.x-x1;
    float dRight  = x2 - gl_FragCoord.x;
    float dBottom = gl_FragCoord.y-y1;
    float dTop    = y2 - gl_FragCoord.y;
    

    float distToBorder = min(min(dLeft, dRight), min(dBottom, dTop));

    bool isBorder = distToBorder <= (BORDER*u_scaleFactor);

    // --- PARAMÉTRISATION DU PÉRIMÈTRE (sens horaire) ---
    float p;

    if ((gl_FragCoord.y-y1) <= (BORDER*u_scaleFactor)) {
        p = gl_FragCoord.x-x1;
    }
    else if ((x2-gl_FragCoord.x) <= (BORDER*u_scaleFactor)) {
        p = w + (gl_FragCoord.y-y1);
    }
    else if ((y2-gl_FragCoord.y) <= (BORDER*u_scaleFactor)) {
        p = w + h + (w - (gl_FragCoord.x-x1));
    }
    else {
        p = w + h + w + (h - (gl_FragCoord.y-y1));
    }

    float perimeter = 2.0 * (w + h);
    float hue = fract(7.0 * p / perimeter + u_time * 0.25);

    vec3 neonColor = hsv2rgb(vec3(hue, 1.0, 1.0));

    // --- INTENSITÉ DU GLOW ---
    float glow = pow(
        clamp(1.0 - distToBorder / (NEON_RADIUS*u_scaleFactor), 0.0, 1.0),
        NEON_POWER
    );

    // --- COULEUR DE BASE ---
    vec3 baseColor;
    if (a ^^ b) baseColor = v_color.xyz;
    else        baseColor = 1.0-v_color.xyz;

    // --- COMPOSITION ---
    vec3 color = baseColor;

    // Glow additif
    color += neonColor * glow * 1.4;

    // Bordure pleine (tube néon)
    if (isBorder) {
        color = neonColor * 1.8;
    }

    out_color = vec4(color, v_color.w);
}
