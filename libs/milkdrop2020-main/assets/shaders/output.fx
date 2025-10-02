
#include "common.fxh"

// fixed function vertex pipeline
VS_OUTPUT VS(VS_INPUT v)
{  
	VS_OUTPUT o;
	o.pos   = mul(xform_position, float4(v.pos.xyz,1));
//    o.color = clamp(v.color, 0, 1);
    o.color = v.color;

    o.uv    = v.uv.xy;
    return o;
}


float4 ConvertLinearToSRGB(float4 c)
{
  //  if (isnan(c)) c = 0.0;
    c = clamp(c, 0, 1);
    return (c < 0.0031308) ? (12.92 * c) : (1.055 * pow(c, 1.0/2.4) - 0.055);
}


// The conversion algorithm from sRGB to linear is:

float4 ConvertSRGBToLinear(float4 c)
{
    return  (c <= 0.04045) ? (c / 12.92) : pow((c + 0.055) / 1.055, 2.4);
}
    




// fixed function pixel shader
float4 PS(VS_OUTPUT v) : COLOR
{
	// sample texture
	float4 tcolor = tex2D(sampler0, v.uv);

	// texture * color
	float4 c = tcolor * v.color;
 
    c.a = 1;

    c.rgb = clamp(c.rgb, 0, 4);

    //c *= 2;
 
    //c = (c >= 1.0) ? c : 0;
 
    //c *= 4;
    
    //c = pow(c, 2.2) * 2;
    

  //  c = clamp(c, 1, 2);
 
	return c;
}


