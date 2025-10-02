


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
    float4 uv      : TEXCOORD0;
    float2 rad_ang  : TEXCOORD1;
};


VS_OUTPUT VS(VS_INPUT v)
{
    VS_OUTPUT o;

    float texel_offset_x = 0;
    float texel_offset_y = 0;

    float tu_orig =  v.pos.x*0.5 + 0.5 + texel_offset_x;
    float tv_orig = -v.pos.y*0.5 + 0.5 + texel_offset_y;


    o.pos   = float4(v.pos.xyz, 1);
    o.color = clamp(v.color, 0, 1);
    o.uv    = float4(v.uv.x, v.uv.y, tu_orig, tv_orig);
    o.rad_ang = v.rad_ang;
    return o;
}


