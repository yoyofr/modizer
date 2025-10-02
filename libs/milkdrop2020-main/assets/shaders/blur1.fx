

struct VS_INPUT
{
    float3 pos       : POSITION;
    float4 color     : COLOR;
    float2 uv        : TEXCOORD0;
};


struct VS_OUTPUT
{
    float4 pos     : POSITION;
    float4 color   : COLOR;
    float2 uv      : TEXCOORD0;
};


VS_OUTPUT VS(VS_INPUT v)
{
    VS_OUTPUT o;
    o.pos   = float4(v.pos.xyz, 1);
    o.color = float4(1,1,1,1);
    o.uv    = v.uv;
    return o;
}



sampler2D sampler_main;
float4 srctexsize; // source texsize (.xy), and inverse (.zw)
float4 wv; // w1..w4
float4 dv; // d1..d4
float fscale;
float fbias;
float w_div;



float4 PS( VS_OUTPUT _v) : COLOR0
{
    float2 uv = _v.uv;

    // LONG HORIZ. PASS 1:
    //const float w[8] = { 4.0, 3.8, 3.5, 2.9, 1.9, 1.2, 0.7, 0.3 };  <- user can specify these
    //const float w1 = w[0] + w[1];
    //const float w2 = w[2] + w[3];
    //const float w3 = w[4] + w[5];
    //const float w4 = w[6] + w[7];
    //const float d1 = 0 + 2*w[1]/w1;
    //const float d2 = 2 + 2*w[3]/w2;
    //const float d3 = 4 + 2*w[5]/w3;
    //const float d4 = 6 + 2*w[7]/w4;
    //const float w_div = 0.5/(w1+w2+w3+w4);
    #define w1 wv.x
    #define w2 wv.y
    #define w3 wv.z
    #define w4 wv.w
    #define d1 dv.x
    #define d2 dv.y
    #define d3 dv.z
    #define d4 dv.w


    // note: if you just take one sample at exactly uv.xy, you get an avg of 4 pixels.
    //float2 uv2 = uv.xy;// + srctexsize.zw*float2(0.5,0.5);
    float2 uv2 = uv.xy + srctexsize.zw*float2(1,1);     // + moves blur UP, LEFT by 1-pixel increments
    
    float3 blur = 
            ( tex2D( sampler_main, uv2 + float2( d1*srctexsize.z,0) ).xyz
            + tex2D( sampler_main, uv2 + float2(-d1*srctexsize.z,0) ).xyz)*w1 +
            ( tex2D( sampler_main, uv2 + float2( d2*srctexsize.z,0) ).xyz
            + tex2D( sampler_main, uv2 + float2(-d2*srctexsize.z,0) ).xyz)*w2 +
            ( tex2D( sampler_main, uv2 + float2( d3*srctexsize.z,0) ).xyz
            + tex2D( sampler_main, uv2 + float2(-d3*srctexsize.z,0) ).xyz)*w3 +
            ( tex2D( sampler_main, uv2 + float2( d4*srctexsize.z,0) ).xyz
            + tex2D( sampler_main, uv2 + float2(-d4*srctexsize.z,0) ).xyz)*w4
            ;
    blur.xyz *= w_div;
    
    blur.xyz = blur.xyz*fscale + fbias;
       
    //ret.xyzw = tex2D(sampler_main, uv + 0*srctexsize.zw);

    return float4(blur, 1);
}
