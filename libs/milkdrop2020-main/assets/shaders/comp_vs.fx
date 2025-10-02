

struct VS_INPUT
{
    float3 pos       : POSITION;
    float4 color     : COLOR;
    float2 uv        : TEXCOORD0;
    float2 rad_ang   : TEXCOORD1;
};


struct VS_OUTPUT
{
    float4 pos     : POSITION;
    float4 color   : COLOR;
    float2 uv      : TEXCOORD0;
    float2 rad_ang  : TEXCOORD1;
};


VS_OUTPUT VS(VS_INPUT v)
{
    VS_OUTPUT o;

    o.pos = float4(v.pos.xyz, 1);
    o.color = clamp(v.color, 0, 1);
    o.uv    = float2(v.uv.x, v.uv.y);
    o.rad_ang = v.rad_ang;
    return o;
}


