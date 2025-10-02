

#include "common.fxh"

float fDecay = 1;

// fixed function vertex pipeline
VS_OUTPUT VS(VS_INPUT v)
{  
	VS_OUTPUT o;

    // flip y upside down
    v.pos.y *= -1;

    // set decay
	o.pos   = mul(xform_position, float4(v.pos.xyz,1));
    o.color = float4(fDecay, fDecay, fDecay, v.color.a);
    o.uv    = v.uv;
    return o;
}

// fixed function pixel shader
float4 PS(VS_OUTPUT v) : COLOR
{
	// sample texture
	float4 tcolor = tex2D(sampler0, v.uv);

	// texture * color
	float4 c = tcolor * v.color;

	// preserve alpha from vertex color
	c.a = v.color.a;
	return c;
}
