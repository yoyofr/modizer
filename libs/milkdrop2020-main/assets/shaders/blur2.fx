

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


//
//
//

sampler2D sampler_main;
float4 srctexsize; // source texsize (.xy), and inverse (.zw)
float4 wv;
float4 dv;
float4 edge_darken;
float w_div;



float4 PS( VS_OUTPUT _v) : COLOR0
{
    float2 uv = _v.uv;

    //SHORT VERTICAL PASS 2:
    //const float w1 = w[0]+w[1] + w[2]+w[3];
    //const float w2 = w[4]+w[5] + w[6]+w[7];
    //const float d1 = 0 + 2*((w[2]+w[3])/w1);
    //const float d2 = 2 + 2*((w[6]+w[7])/w2);
    //const float w_div = 1.0/((w1+w2)*2);





    #define w1 wv.x
    #define w2 wv.y


    #define d1 dv.x
    #define d2 dv.y
    


    // note: if you just take one sample at exactly uv.xy, you get an avg of 4 pixels.
    //float2 uv2 = uv.xy;// + srctexsize.zw*float2(-0.5,-0.5);
    float2 uv2 = uv.xy + srctexsize.zw*float2(1,0);     // + moves blur UP, LEFT by TWO-pixel increments! (since texture is 1/2 the size of blur1_ps)

    float3 blur = 
            ( tex2D( sampler_main, uv2 + float2(0, d1*srctexsize.w) ).xyz
            + tex2D( sampler_main, uv2 + float2(0,-d1*srctexsize.w) ).xyz)*w1 +
            ( tex2D( sampler_main, uv2 + float2(0, d2*srctexsize.w) ).xyz
            + tex2D( sampler_main, uv2 + float2(0,-d2*srctexsize.w) ).xyz)*w2
            ;
    blur.xyz *= w_div;

    // tone it down at the edges:  (only happens on 1st X pass!)
    float t = min( min(uv.x, uv.y), 1-max(uv.x,uv.y) );
    t = sqrt(t);
    t = edge_darken.x + edge_darken.y*saturate(t*edge_darken.z);
    blur.xyz *= t;
    
    //ret.xyzw = tex2D(sampler_main, uv + 0*srctexsize.zw);
    return float4(blur, 1);
}
