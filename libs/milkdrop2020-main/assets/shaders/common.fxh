
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
};




sampler2D sampler0;
float4x4 xform_position;

