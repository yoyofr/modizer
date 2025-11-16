#include "ShaderPreprocessor.h"
#include <iostream>

int main() {
    ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
    preprocessor.setVerbose(true);
    
    std::string test = "//Kreisverkehr\n\
    uv3 = float2 ((_rad_ang.x)/(_c0).y*1.07,(_rad_ang.y)/ 4.0* 2.0);\n\
    uv3.y += (int(uv3.x* 32.0- 12.0))*0.023*(_c2.x);\n\
    ret.b +=  (( 1.0*tex2D (sampler_pw_noise_lq, uv3).r) > 0.94) *(streetr>slim) * 0.8;;\n\
";
    
    std::cout << "Input: " << test << std::endl;
    std::cout << "Length: " << test.length() << std::endl;
    std::cout << std::endl;
    
    // Show character positions
    for (size_t i = 0; i < test.length(); ++i) {
        if (test[i] == '(' || test[i] == ')') {
            std::cout << "Position " << i << ": '" << test[i] << "'" << std::endl;
        }
    }
    std::cout << std::endl;
    
    std::string result = preprocessor.removeRedundantParentheses(test);
    
    std::cout << std::endl;
    std::cout << "Result: " << result << std::endl;
    std::cout << "Length: " << result.length() << std::endl;
    std::cout << "Changed: " << (test != result ? "YES" : "NO") << std::endl;
    
    return 0;
}
