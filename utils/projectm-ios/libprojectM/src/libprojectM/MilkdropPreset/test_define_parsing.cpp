// Test to verify define parsing works correctly
#include "ShaderPreprocessor.h"
#include <iostream>

int main() {
    std::string testCode = R"(
#define lum(x) (dot(x,float3(0.32,0.49,0.29)))
#define tex2d tex2D
#define rad _rad_ang.x
#define sat saturate
#define go if (r2.w>0) {ret1=r2;}  if (lum(ret1)==0) {ret1=r2;}
sampler sampler_pw_noise_lq;
float4 ring (float2 uvi, float r, float a)
{ float w=r*0.04, ri=r-w, ra=r+w;
  float h0, h1, h2, h3; float2 rs;  
  float4 res=0;
  h3 = abs(cos(a));
  res.gb *= (res.r==0)*h3;
  res.b  *= (res.g==0);   
  res.w = between (length(rs),ra,ri)* (uvi.x*sign(cos(a)*sin(a))>-w) || (res.g);
 return res;
}
 

void PS(float4 _vDiffuse : COLOR, float2 _uv : TEXCOORD0, float2 _rad_ang : TEXCOORD1, out float4 _return_value : COLOR)
 {
float3 ret = 0;

uv0=(uv-0.5)*aspect.xy*.6;
uv1=rotuv(uv0,q11,q21);
ret1 = ring (uv1,0.06,q1); 
float2 uv3=uv1;
uv1=rotuv(uv0,q12,q22); 
r2 = ring (uv1,0.08,q2); go
uv1=rotuv(uv0,q13,q23); 
r2 = ring (uv1,0.1,q3); go
uv1=rotuv(uv0,q14, q24); 
r2 = ring (uv1,0.12, q4); go
uv1=rotuv(uv0,q15, q25); 
r2 = ring (uv1,0.14,q5); go

}
)";

    ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
    preprocessor.setVerbose(true);
    
    std::cout << "=== ORIGINAL CODE ===" << std::endl;
    std::cout << testCode << std::endl;
    
    std::cout << "\n=== PROCESSING ===" << std::endl;
    std::string result = preprocessor.processDefines(testCode);
    
    std::cout << "\n=== RESULT ===" << std::endl;
    std::cout << result << std::endl;
    
    return 0;
}
