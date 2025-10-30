//
//  PreprocessorCleanup.cpp
//  projectm-ios
//
//  Created by Yohann Magnien David on 30/10/2025.
//

// HLSLPreprocessorCleanup.cpp
#include "HLSLPreprocessorCleanup.hpp"
#include <iostream>
#include <regex>

std::string HLSLPreprocessorCleanup::removeComments(const std::string& code) {
    std::string result = code;
    
    // Remove single-line comments
    result = std::regex_replace(result, std::regex("//.*"), "");
    
    // Remove multi-line comments
    std::regex multiComment(R"(/\*[\s\S]*?\*/)");
    result = std::regex_replace(result, multiComment, "");
    
    return result;
}

std::string HLSLPreprocessorCleanup::removeStringLiterals(const std::string& code) {
    std::string result;
    bool inString = false;
    char prev = '\0';
    
    for (size_t i = 0; i < code.length(); i++) {
        char c = code[i];
        
        if (c == '"' && prev != '\\') {
            inString = !inString;
            result += ' ';
        } else if (!inString) {
            result += c;
        } else {
            result += ' ';
        }
        
        prev = c;
    }
    
    return result;
}

bool HLSLPreprocessorCleanup::isVoidType(const std::string& returnType) {
    std::string trimmed = returnType;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    return trimmed == "void";
}

bool HLSLPreprocessorCleanup::hasReturnStatement(const std::string& functionBody) {
    std::regex returnPattern(R"(\breturn\b)");
    return std::regex_search(functionBody, returnPattern);
}

std::vector<HLSLPreprocessorCleanup::Function> HLSLPreprocessorCleanup::extractFunctions(const std::string& code) {
    std::vector<Function> functions;
    
    // Pattern to match HLSL function definitions
    // HLSL types include: float, float2, float3, float4, int, uint, bool, matrix types, etc.
    // Also supports custom struct types and semantic annotations
    std::regex funcPattern(
        R"((\w+(?:<[^>]+>)?(?:\s*\[[\d\s]*\])?)\s+)"  // return type (with optional template/array)
        R"((\w+)\s*\([^)]*\)\s*(?::\s*\w+\s*)?\{)"    // function name, params, optional semantic
    );

    std::string cleanCode = removeComments(code);
    cleanCode = removeStringLiterals(cleanCode);
    cleanCode=code;
    
    auto begin = std::sregex_iterator(cleanCode.begin(), cleanCode.end(), funcPattern);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        Function func;
        func.returnType = match[1].str();
        func.name = match[2].str();
        func.startPos = match.position();
        
        // Find matching closing brace
        size_t braceStart = match.position() + match.length() - 1;
        int braceCount = 1;
        size_t pos = braceStart + 1;
        
        while (pos < cleanCode.length() && braceCount > 0) {
            if (cleanCode[pos] == '{') braceCount++;
            else if (cleanCode[pos] == '}') braceCount--;
            pos++;
        }
        
        func.endPos = pos;
        func.fullText = code.substr(func.startPos, func.endPos - func.startPos);
        
        // Extract function body (between braces)
        std::string body = cleanCode.substr(braceStart + 1, pos - braceStart - 2);
        func.hasReturn = hasReturnStatement(body);
        
        functions.push_back(func);
    }
    
    return functions;
}

std::string HLSLPreprocessorCleanup::process(const std::string& code) {
    std::vector<Function> functions = extractFunctions(code);
    std::vector<bool> shouldRemove(code.length(), false);
    
    // Mark functions for removal
    for (const auto& func : functions) {
        if (!isVoidType(func.returnType) && !func.hasReturn) {
            std::cout << "Removing HLSL function '" << func.name
                      << "' (return type: " << func.returnType
                      << ") - missing return statement\n";
            
            for (size_t i = func.startPos; i < func.endPos; i++) {
                shouldRemove[i] = true;
            }
        }
    }
    
    // Build output without removed functions
    std::string result;
    for (size_t i = 0; i < code.length(); i++) {
        if (!shouldRemove[i]) {
            result += code[i];
        }
    }
    
    return result;
}
