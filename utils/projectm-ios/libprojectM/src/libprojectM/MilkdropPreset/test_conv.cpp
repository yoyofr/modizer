#include "ShaderPreprocessor.h"
#include <iostream>

int main()
{
    std::string testCode = R"(
float3 ret1, neu;
float2 uv1, uv2,uv4,rsk,dz1,Kugel1;
float ang2, c, s,rad1,mask1,zoom;
static int n = 1;
static int anz = 4;
int m;

shader_body {

uv = (uv-.5)*aspect.xy;    
//Kugel1
rsk = uv+.2 ;
dz1 = normalize(rsk);
rad1 = 16*length (rsk) ;
uv4 =  tan(rad1)*dz1;
mask1 = saturate(10-7*rad1);
Kugel1 = uv4*mask1;

float2 rs = float2(.1/rad*q15,ang*.2*q22);
float noise = tex2D (sampler_noise_mq,rs);

uv /= 1+ q8*length(uv); //zoom
uv *= 1-q28%2/4;
uv *= 1+q5*noise*rad;
uv *=1; 
ret1 = 0;
while (n <= anz) {
    m = n%2;
    ang2 = 6.28*n/anz*q14 + 0*(n%2-.5)*q9 + q28*3.14/4;//##
    c = cos(ang2);
    s = sin(ang2);
    uv2.x =  uv.x*c - uv.y*s;
    uv2.y =  uv.x*s + uv.y*c ;
    uv2 /= 1+ q26*length(uv)*m; //flash zoom
    uv2 *= 1+(m-.5)*q18; //waber
    if (q3*m == 1) {uv2=uv2.yx;}
    neu = tex2D(sampler_main, uv2 + 0.5); //#
    ret1 = max(ret1,neu-.0);
    n++;
}
ret1 *= ((1-q7)+q7*q25/2);
ret = 1-exp(-ret1*4);
ret = pow(ret,1.5);
ret = lerp(lum(ret),ret,saturate(q20/3));
//ret = GetPixel(uv+.5);//#
ret-=roam_sin.wyx*roam_cos.zwx;
}
)";

    std::cout << "Original code:\n"
              << testCode << "\n\n";

    ShaderPreprocessor preProcessor(ShaderLanguage::HLSL);
    preProcessor.setVerbose(true);
    std::string newCode = preProcessor.preprocess(testCode);

    std::cout << "\n\nFixed code:\n"
              << newCode << "\n";

    return 0;
}
