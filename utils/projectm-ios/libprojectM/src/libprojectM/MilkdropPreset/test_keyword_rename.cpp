//
//  test_keyword_rename.cpp
//  Test for renameKeywordsAsVariables functionality
//
//  This demonstrates how to use the new keyword renaming feature
//

#include "ShaderPreprocessor.h"
#include <iostream>

int main() {
    // Create a preprocessor for HLSL
    ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
    preprocessor.setVerbose(true);
    
    // Test case 1: Basic sample variable usage
    std::string testShader1 = R"(
float4 main(float2 uv : TEXCOORD) : COLOR {
    float3 sample = tex2D(sampler_main, uv);
    ret = sample*sample*sample;
    return float4(ret, 1.0);
}
)";
    
    std::cout << "=== Test Case 1: Basic sample variable ===" << std::endl;
    std::cout << "Before:" << std::endl << testShader1 << std::endl;
    
    std::string processed1 = preprocessor.preprocess(testShader1);
    std::cout << "After:" << std::endl << processed1 << std::endl;
    
    // Test case 2: Multiple keywords
    std::string testShader2 = R"(
float4 main(float2 uv : TEXCOORD) : COLOR {
    float3 sample = tex2D(sampler_main, uv);
    float3 color = sample * 2.0;
    return float4(color, 1.0);
}
)";
    
    std::cout << "\n=== Test Case 2: Multiple usages ===" << std::endl;
    std::cout << "Before:" << std::endl << testShader2 << std::endl;
    
    std::string processed2 = preprocessor.preprocess(testShader2);
    std::cout << "After:" << std::endl << processed2 << std::endl;
    
    // Test case 3: Using renameKeywordsAsVariables directly with custom keywords
    std::string testShader3 = R"(
float4 main(float2 uv : TEXCOORD) : COLOR {
    float3 input = tex2D(sampler_main, uv);
    float3 output = input * 2.0;
    return float4(output, 1.0);
}
)";
    
    std::cout << "\n=== Test Case 3: Custom keywords (input, output) ===" << std::endl;
    std::cout << "Before:" << std::endl << testShader3 << std::endl;
    
    std::vector<std::string> customKeywords = {"input", "output"};
    std::string processed3 = preprocessor.renameKeywordsAsVariables(testShader3, customKeywords, "_var");
    std::cout << "After:" << std::endl << processed3 << std::endl;
    
    // Test case 4: Should not touch function calls
    std::string testShader4 = R"(
float3 sample(float2 uv) {
    return tex2D(sampler_main, uv);
}

float4 main(float2 uv : TEXCOORD) : COLOR {
    float3 result = sample(uv);  // This should NOT be renamed
    return float4(result, 1.0);
}
)";
    
    std::cout << "\n=== Test Case 4: Should not rename function names ===" << std::endl;
    std::cout << "Before:" << std::endl << testShader4 << std::endl;
    
    std::string processed4 = preprocessor.preprocess(testShader4);
    std::cout << "After:" << std::endl << processed4 << std::endl;
    
    return 0;
}
