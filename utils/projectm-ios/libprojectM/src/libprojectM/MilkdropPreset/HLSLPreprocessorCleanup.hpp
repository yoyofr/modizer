//
//  PreprocessorCleanup.hpp
//  projectm-ios
//
//  Created by Yohann Magnien David on 30/10/2025.
//

// HLSLPreprocessorCleanup.h
#ifndef HLSL_PREPROCESSOR_CLEANUP_H
#define HLSL_PREPROCESSOR_CLEANUP_H

#include <string>
#include <vector>

class HLSLPreprocessorCleanup {
private:
    struct Function {
        std::string fullText;
        std::string returnType;
        std::string name;
        size_t startPos;
        size_t endPos;
        bool hasReturn;
    };

    std::string removeComments(const std::string& code);
    std::string removeStringLiterals(const std::string& code);
    bool isVoidType(const std::string& returnType);
    bool hasReturnStatement(const std::string& functionBody);
    std::vector<Function> extractFunctions(const std::string& code);

public:
    // Process HLSL shader code string and return cleaned version
    std::string process(const std::string& code);
};

#endif // HLSL_PREPROCESSOR_CLEANUP_H
