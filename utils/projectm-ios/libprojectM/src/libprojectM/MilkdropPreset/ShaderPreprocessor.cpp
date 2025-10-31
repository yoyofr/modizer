//
//  ShaderPreprocessor.cpp
//  projectm-ios
//
//  Created by Yohann Magnien David on 30/10/2025.
//

#include "ShaderPreprocessor.h"
#include <regex>
#include <algorithm>

ShaderPreprocessor::ShaderPreprocessor(ShaderLanguage language)
    : m_language(language) {
}

void ShaderPreprocessor::setLanguage(ShaderLanguage language) noexcept {
    m_language = language;
}

std::string ShaderPreprocessor::removeInvalidFunctions(const std::string& shaderSource) {
    std::vector<FunctionInfo> functions = extractFunctions(shaderSource);
    std::string result = shaderSource;
    
    // Process functions in reverse order to maintain correct indices
    for (auto it = functions.rbegin(); it != functions.rend(); ++it) {
        if (it->shouldReturnValue && !it->hasReturn) {
            // Remove this function from the source
            result.erase(it->startPos, it->length);
        }
    }
    
    return result;
}

std::string ShaderPreprocessor::preprocess(const std::string& shaderSource) {
    std::string result = removeInvalidFunctions(shaderSource);
    result = fixVariableShadowing(result);
    result = fixDivisionByZero(result);
    return result;
}

std::string ShaderPreprocessor::fixVariableShadowing(const std::string& shaderSource) {
    std::vector<ShadowingInfo> shadowingCases = detectShadowing(shaderSource);
    
    if (shadowingCases.empty()) {
        return shaderSource;
    }
    
    std::string result = shaderSource;
    int offset = 0;
    
    // Process in order (already sorted by position)
    for (const auto& shadow : shadowingCases) {
        // Calculate the new variable name
        std::string newVarName = shadow.varName + "_" + shadow.varType;
        
        // Replace the variable name in the declaration
        size_t declStart = shadow.declarationStart + offset;
        size_t declEnd = shadow.declarationEnd + offset;
        
        // Find the variable name in the declaration and replace it
        std::string declaration = result.substr(declStart, declEnd - declStart);
        
        // Create regex to match the variable name after the type
        std::string pattern = R"(\b)" + shadow.varType + R"(\s+)" + shadow.varName + R"(\b)";
        std::regex varRegex(pattern);
        std::string replacement = shadow.varType + " " + newVarName;
        
        std::string newDeclaration = std::regex_replace(declaration, varRegex, replacement, std::regex_constants::format_first_only);
        
        // Replace in result
        result.replace(declStart, declEnd - declStart, newDeclaration);
        
        // Now replace all subsequent uses of the old variable name with the new one
        // Start searching after the declaration
        size_t searchStart = declStart + newDeclaration.length();
        
        // Find the end of the scope (simplified: end of string or closing brace at same level)
        // For now, replace until end of string
        std::string remainingCode = result.substr(searchStart);
        
        // Replace the variable name as a whole word
        std::string wordPattern = R"(\b)" + shadow.varName + R"(\b)";
        std::regex wordRegex(wordPattern);
        std::string replacedRemaining = std::regex_replace(remainingCode, wordRegex, newVarName);
        
        result.replace(searchStart, remainingCode.length(), replacedRemaining);
        
        // Update offset
        offset += static_cast<int>(newVarName.length()) - static_cast<int>(shadow.varName.length());
    }
    
    return result;
}

std::vector<ShaderPreprocessor::ShadowingInfo> ShaderPreprocessor::detectShadowing(const std::string& source) {
    std::vector<ShadowingInfo> shadowingCases;
    std::vector<std::string> types = getShaderTypes();
    
    // Build regex pattern for variable declarations that reference themselves
    // Pattern: type varName = varName.member or type varName = ...varName...
    for (const auto& type : types) {
        // Match: type varName = ...
        std::string pattern = R"(\b)" + type + R"(\s+([a-zA-Z_]\w*)\s*=\s*([^;]+);)";
        std::regex declRegex(pattern);
        
        auto searchStart = source.cbegin();
        std::smatch match;
        
        while (std::regex_search(searchStart, source.cend(), match, declRegex)) {
            std::string varName = match[1].str();
            std::string initExpression = match[2].str();
            
            // Check if the initialization expression uses the same variable name
            // Look for varName.something or varName[something] or varName as standalone
            std::string usagePattern = R"(\b)" + varName + R"(\s*[\.\[])";
            std::regex usageRegex(usagePattern);
            
            if (std::regex_search(initExpression, usageRegex)) {
                // Found shadowing!
                ShadowingInfo info;
                info.varType = type;
                info.varName = varName;
                info.originalName = varName;
                info.declarationStart = static_cast<size_t>(match.position(0) + std::distance(source.cbegin(), searchStart));
                info.declarationEnd = info.declarationStart + match.length(0);
                
                shadowingCases.push_back(std::move(info));
            }
            
            searchStart = match.suffix().first;
        }
    }
    
    return shadowingCases;
}

std::vector<std::string> ShaderPreprocessor::getShaderTypes() const {
    if (m_language == ShaderLanguage::GLSL) {
        return {
            "float", "int", "uint", "bool",
            "vec2", "vec3", "vec4",
            "ivec2", "ivec3", "ivec4",
            "uvec2", "uvec3", "uvec4",
            "bvec2", "bvec3", "bvec4",
            "mat2", "mat3", "mat4",
            "mat2x2", "mat2x3", "mat2x4",
            "mat3x2", "mat3x3", "mat3x4",
            "mat4x2", "mat4x3", "mat4x4"
        };
    } else { // HLSL
        return {
            "float", "float2", "float3", "float4",
            "int", "int2", "int3", "int4",
            "uint", "uint2", "uint3", "uint4",
            "bool", "bool2", "bool3", "bool4",
            "half", "half2", "half3", "half4",
            "double", "double2", "double3", "double4",
            "float1x1", "float1x2", "float1x3", "float1x4",
            "float2x1", "float2x2", "float2x3", "float2x4",
            "float3x1", "float3x2", "float3x3", "float3x4",
            "float4x1", "float4x2", "float4x3", "float4x4",
            "matrix", "vector"
        };
    }
}

std::string ShaderPreprocessor::fixDivisionByZero(const std::string& shaderSource) {
    std::vector<ForLoopInfo> dangerousLoops = detectDivisionByZeroInLoops(shaderSource);
    
    if (dangerousLoops.empty()) {
        return shaderSource;
    }
    
    std::string result = shaderSource;
    int offset = 0;
    
    // Process loops in order
    for (const auto& loop : dangerousLoops) {
        size_t forStart = loop.forStatementStart + offset;
        size_t forEnd = loop.forStatementEnd + offset;
        
        // Extract the for statement
        std::string forStatement = result.substr(forStart, forEnd - forStart);
        
        // Replace the initialization value (0 -> 1)
        // Pattern: varType varName = 0
        std::string pattern = R"(\b)" + loop.loopVariable + R"(\s*=\s*0\b)";
        std::regex initRegex(pattern);
        std::string replacement = loop.loopVariable + " = 1";
        
        std::string newForStatement = std::regex_replace(forStatement, initRegex, replacement, std::regex_constants::format_first_only);
        
        // Replace in result
        result.replace(forStart, forEnd - forStart, newForStatement);
        
        // Update offset
        offset += static_cast<int>(newForStatement.length()) - static_cast<int>(forStatement.length());
    }
    
    return result;
}

std::vector<ShaderPreprocessor::ForLoopInfo> ShaderPreprocessor::detectDivisionByZeroInLoops(const std::string& source) {
    std::vector<ForLoopInfo> dangerousLoops;
    
    // Pattern to match for loops: for (type var = 0; condition; increment)
    std::regex forRegex(R"(\bfor\s*\(\s*(\w+)\s+([a-zA-Z_]\w*)\s*=\s*0\s*;[^;]+;[^)]+\))");
    
    auto searchStart = source.cbegin();
    std::smatch match;
    
    while (std::regex_search(searchStart, source.cend(), match, forRegex)) {
        std::string loopVar = match[2].str();
        size_t forStart = static_cast<size_t>(match.position(0) + std::distance(source.cbegin(), searchStart));
        size_t forEnd = forStart + match.length(0);
        
        // Find the loop body
        auto bodyOpt = findLoopBody(source, forEnd);
        if (!bodyOpt.has_value()) {
            searchStart = match.suffix().first;
            continue;
        }
        
        auto [bodyStart, bodyEnd] = bodyOpt.value();
        std::string loopBody = source.substr(bodyStart, bodyEnd - bodyStart);
        
        // Check if the loop variable is used in a division
        std::string divPattern = R"(/\s*)" + loopVar + R"(\b)";
        std::regex divRegex(divPattern);
        
        if (std::regex_search(loopBody, divRegex)) {
            // Found a division by the loop variable that starts at 0!
            ForLoopInfo info;
            info.loopVariable = loopVar;
            info.forStatementStart = forStart;
            info.forStatementEnd = forEnd;
            info.loopBodyStart = bodyStart;
            info.loopBodyEnd = bodyEnd;
            info.initValue = "0";
            
            dangerousLoops.push_back(std::move(info));
        }
        
        searchStart = match.suffix().first;
    }
    
    return dangerousLoops;
}

std::optional<std::pair<size_t, size_t>> ShaderPreprocessor::findLoopBody(const std::string& source, size_t forEnd) const {
    // Skip whitespace after the for statement
    size_t pos = forEnd;
    while (pos < source.length() && std::isspace(source[pos])) {
        ++pos;
    }
    
    if (pos >= source.length()) {
        return std::nullopt;
    }
    
    // Check if it's a block statement or single statement
    if (source[pos] == '{') {
        // Block statement - find matching brace
        auto endOpt = findMatchingBrace(source, pos);
        if (!endOpt.has_value()) {
            return std::nullopt;
        }
        return std::make_pair(pos, endOpt.value() + 1);
    } else {
        // Single statement - find the semicolon
        size_t endPos = source.find(';', pos);
        if (endPos == std::string::npos) {
            return std::nullopt;
        }
        return std::make_pair(pos, endPos + 1);
    }
}

std::vector<ShaderPreprocessor::FunctionInfo> ShaderPreprocessor::extractFunctions(const std::string& source) {
    std::vector<FunctionInfo> functions;
    
    // Regex to match function declarations
    // Matches: returnType functionName(params) { ... }
    std::regex funcRegex(R"((void|\w+)\s+(\w+)\s*\([^)]*\)\s*\{)");
    
    auto searchStart = source.cbegin();
    std::smatch match;
    
    while (std::regex_search(searchStart, source.cend(), match, funcRegex)) {
        FunctionInfo func;
        func.startPos = static_cast<size_t>(match.position(0) + std::distance(source.cbegin(), searchStart));
        func.returnType = match[1].str();
        func.shouldReturnValue = (func.returnType != "void");
        
        // Find the matching closing brace
        size_t bodyStart = func.startPos + match.length(0);
        auto bodyEndOpt = findMatchingBrace(source, bodyStart - 1);
        
        if (!bodyEndOpt.has_value()) {
            searchStart = match.suffix().first;
            continue;
        }
        
        size_t bodyEnd = bodyEndOpt.value();
        func.length = bodyEnd - func.startPos + 1;
        func.body = source.substr(bodyStart, bodyEnd - bodyStart);
        func.hasReturn = checkForReturn(func.body);
        
        functions.push_back(std::move(func));
        searchStart = source.cbegin() + bodyEnd + 1;
    }
    
    return functions;
}

std::optional<size_t> ShaderPreprocessor::findMatchingBrace(const std::string& source, size_t openBracePos) const {
    int braceCount = 0;
    bool inString = false;
    bool inComment = false;
    bool inLineComment = false;
    
    for (size_t i = openBracePos; i < source.length(); ++i) {
        char c = source[i];
        char next = (i + 1 < source.length()) ? source[i + 1] : '\0';
        
        // Handle line comments
        if (!inString && !inComment && c == '/' && next == '/') {
            inLineComment = true;
            ++i;
            continue;
        }
        
        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
            }
            continue;
        }
        
        // Handle block comments
        if (!inString && !inLineComment && c == '/' && next == '*') {
            inComment = true;
            ++i;
            continue;
        }
        
        if (inComment) {
            if (c == '*' && next == '/') {
                inComment = false;
                ++i;
            }
            continue;
        }
        
        // Handle strings
        if (c == '"' && (i == 0 || source[i - 1] != '\\')) {
            inString = !inString;
            continue;
        }
        
        if (inString) {
            continue;
        }
        
        // Count braces
        if (c == '{') {
            ++braceCount;
        } else if (c == '}') {
            --braceCount;
            if (braceCount == 0) {
                return i;
            }
        }
    }
    
    return std::nullopt;
}

bool ShaderPreprocessor::checkForReturn(const std::string& body) const {
    // Remove comments and strings first to avoid false positives
    std::string cleanBody = removeCommentsAndStrings(body);
    
    // Look for return statements that return a value
    std::regex returnRegex(R"(\breturn[\s+('"][^;]+;)");
    return std::regex_search(cleanBody, returnRegex);
}

std::string ShaderPreprocessor::removeCommentsAndStrings(const std::string& source) const {
    std::string result;
    result.reserve(source.length());
    
    bool inString = false;
    bool inComment = false;
    bool inLineComment = false;
    
    for (size_t i = 0; i < source.length(); ++i) {
        char c = source[i];
        char next = (i + 1 < source.length()) ? source[i + 1] : '\0';
        
        if (!inString && !inComment && c == '/' && next == '/') {
            inLineComment = true;
            result += ' ';
            ++i;
            continue;
        }
        
        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
                result += '\n';
            } else {
                result += ' ';
            }
            continue;
        }
        
        if (!inString && !inLineComment && c == '/' && next == '*') {
            inComment = true;
            result += ' ';
            ++i;
            continue;
        }
        
        if (inComment) {
            if (c == '*' && next == '/') {
                inComment = false;
                result += ' ';
                ++i;
            } else {
                result += (c == '\n') ? '\n' : ' ';
            }
            continue;
        }
        
        if (c == '"' && (i == 0 || source[i - 1] != '\\')) {
            inString = !inString;
            result += ' ';
            continue;
        }
        
        if (inString) {
            result += ' ';
        } else {
            result += c;
        }
    }
    
    return result;
}
