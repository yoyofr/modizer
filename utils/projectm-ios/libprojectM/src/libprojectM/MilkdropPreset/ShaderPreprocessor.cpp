//
//  ShaderPreprocessor.cpp
//  projectm-ios
//
//  Created by Yohann Magnien David on 30/10/2025.
//

#include "ShaderPreprocessor.h"
#include <HLSLTypeFixer.h>
#include <regex>
#include <algorithm>
#include <iostream>

ShaderPreprocessor::ShaderPreprocessor(ShaderLanguage language)
    : m_language(language), m_verbose(false) {
}

void ShaderPreprocessor::setLanguage(ShaderLanguage language) noexcept {
    m_language = language;
}

void ShaderPreprocessor::setVerbose(bool verbose) noexcept {
    m_verbose = verbose;
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

std::vector<ShaderPreprocessor::FunctionInfo> ShaderPreprocessor::extractFunctions(const std::string& source) {
    std::vector<FunctionInfo> functions;
    functions.reserve(16); // Pre-allocate to avoid reallocation
    
    // Manual parsing instead of regex for better performance
    size_t pos = 0;
    while (pos < source.length()) {
        // Look for potential function pattern: word followed by word then (
        size_t openParen = source.find('(', pos);
        if (openParen == std::string::npos || openParen < 2) break;
        
        // Find the function name (work backwards from parenthesis)
        size_t nameEnd = openParen;
        while (nameEnd > 0 && std::isspace(source[nameEnd - 1])) --nameEnd;
        if (nameEnd == 0) break;
        
        size_t nameStart = nameEnd;
        while (nameStart > 0 && (std::isalnum(source[nameStart - 1]) || source[nameStart - 1] == '_')) {
            --nameStart;
        }
        
        if (nameStart >= nameEnd) {
            pos = openParen + 1;
            continue;
        }

        // Get the function name
        std::string functionName = source.substr(nameStart, nameEnd - nameStart);

        // Skip control flow keywords (if, while, for, switch, etc.)
        if (functionName == "if" || functionName == "while" || functionName == "for" ||
            functionName == "switch" || functionName == "do") {
            pos = openParen + 1;
            continue;
        }

        // Find the return type (work backwards from function name)
        size_t typeEnd = nameStart;
        while (typeEnd > 0 && std::isspace(source[typeEnd - 1])) --typeEnd;
        if (typeEnd == 0) {
            pos = openParen + 1;
            continue;
        }
        
        size_t typeStart = typeEnd;
        while (typeStart > 0 && (std::isalnum(source[typeStart - 1]) || source[typeStart - 1] == '_')) {
            --typeStart;
        }
        
        if (typeStart >= typeEnd) {
            pos = openParen + 1;
            continue;
        }
        
        std::string returnType = source.substr(typeStart, typeEnd - typeStart);
        
        // Find closing parenthesis
        int parenDepth = 1;
        size_t closeParen = openParen + 1;
        while (closeParen < source.length() && parenDepth > 0) {
            if (source[closeParen] == '(') ++parenDepth;
            else if (source[closeParen] == ')') --parenDepth;
            ++closeParen;
        }
        
        if (parenDepth != 0) {
            pos = openParen + 1;
            continue;
        }
        
        // Skip whitespace to find opening brace
        size_t bracePos = closeParen;
        while (bracePos < source.length() && std::isspace(source[bracePos])) ++bracePos;
        
        if (bracePos >= source.length() || source[bracePos] != '{') {
            pos = closeParen;
            continue;
        }
        
        // Found a function! Now find matching closing brace
        auto bodyEndOpt = findMatchingBrace(source, bracePos);
        if (!bodyEndOpt.has_value()) {
            pos = bracePos + 1;
            continue;
        }
        
        FunctionInfo func;
        func.startPos = typeStart;
        func.returnType = returnType;
        func.shouldReturnValue = (returnType != "void");
        
        size_t bodyEnd = bodyEndOpt.value();
        func.length = bodyEnd - typeStart + 1;
        func.body = source.substr(bracePos + 1, bodyEnd - bracePos - 1);
        func.hasReturn = checkForReturn(func.body);
        
        functions.push_back(std::move(func));
        pos = bodyEnd + 1;
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
        
        if (c == '"' && (i == 0 || source[i - 1] != '\\')) {
            inString = !inString;
            continue;
        }
        
        if (inString) {
            continue;
        }
        
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
    // Fast path: scan for "return" keyword without regex
    size_t pos = 0;
    while ((pos = body.find("return", pos)) != std::string::npos) {
        // Check if it's a whole word
        if (pos > 0 && (std::isalnum(body[pos - 1]) || body[pos - 1] == '_')) {
            pos += 6;
            continue;
        }
        
        size_t afterReturn = pos + 6;
        if (afterReturn < body.length() && (std::isalnum(body[afterReturn]) || body[afterReturn] == '_')) {
            pos += 6;
            continue;
        }
        
        // Skip whitespace
        while (afterReturn < body.length() && std::isspace(body[afterReturn])) {
            ++afterReturn;
        }
        
        // Check if there's something after return (not just a semicolon)
        if (afterReturn < body.length() && body[afterReturn] != ';') {
            return true; // Found return with a value
        }
        
        pos += 6;
    }
    
    return false;
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
            result += ' ';  // Add space for second '/' too
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
            result += ' ';  // Add space for '*' too
            ++i;
            continue;
        }
        
        if (inComment) {
            if (c == '*' && next == '/') {
                inComment = false;
                result += ' ';
                result += ' ';  // Add space for '/' too
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

std::string ShaderPreprocessor::renameKeywordsAsVariables(const std::string& shaderSource,
                                                          const std::vector<std::string>& keywords,
                                                          const std::string& suffix) {
    if (keywords.empty()) {
        return shaderSource;
    }
    
    std::vector<KeywordUsageInfo> keywordUsages = detectKeywordUsages(shaderSource, keywords, suffix);
    
    if (keywordUsages.empty()) {
        return shaderSource;
    }
    
    std::string result = shaderSource;
    int offset = 0;
    
    // Process in reverse order to maintain correct indices
    for (auto it = keywordUsages.rbegin(); it != keywordUsages.rend(); ++it) {
        const auto& usage = *it;
        int localOffset = 0;
        
        // First, rename the declaration
        size_t declStart = usage.declarationStart + offset;
        size_t declEnd = usage.declarationEnd + offset;
        std::string declaration = result.substr(declStart, declEnd - declStart);
        
        // Find the keyword in the declaration and replace it
        size_t keywordPos = declaration.find(usage.keyword);
        if (keywordPos != std::string::npos) {
            // Make sure it's a whole word
            bool isWordStart = (keywordPos == 0 || !std::isalnum(declaration[keywordPos - 1]) && declaration[keywordPos - 1] != '_');
            bool isWordEnd = (keywordPos + usage.keyword.length() >= declaration.length() ||
                            !std::isalnum(declaration[keywordPos + usage.keyword.length()]) &&
                            declaration[keywordPos + usage.keyword.length()] != '_');
            
            if (isWordStart && isWordEnd) {
                declaration.replace(keywordPos, usage.keyword.length(), usage.newName);
                result.replace(declStart, declEnd - declStart, declaration);
                localOffset = static_cast<int>(usage.newName.length()) - static_cast<int>(usage.keyword.length());
            }
        }
        
        // Now replace all usages after the declaration
        size_t searchStart = declEnd + localOffset;
        std::string remainingCode = result.substr(searchStart);
        
        size_t pos = 0;
        while (pos < remainingCode.length()) {
            pos = remainingCode.find(usage.keyword, pos);
            if (pos == std::string::npos) break;
            
            // Check if it's a whole word
            bool isWordStart = (pos == 0 || !std::isalnum(remainingCode[pos - 1]) && remainingCode[pos - 1] != '_');
            bool isWordEnd = (pos + usage.keyword.length() >= remainingCode.length() ||
                            !std::isalnum(remainingCode[pos + usage.keyword.length()]) &&
                            remainingCode[pos + usage.keyword.length()] != '_');
            
            if (isWordStart && isWordEnd) {
                remainingCode.replace(pos, usage.keyword.length(), usage.newName);
                localOffset += static_cast<int>(usage.newName.length()) - static_cast<int>(usage.keyword.length());
                pos += usage.newName.length();
            } else {
                pos += usage.keyword.length();
            }
        }
        
        result.replace(searchStart, result.length() - searchStart, remainingCode);
        offset += localOffset;
    }
    
    return result;
}

std::vector<ShaderPreprocessor::KeywordUsageInfo> ShaderPreprocessor::detectKeywordUsages(
    const std::string& source,
    const std::vector<std::string>& keywords,
    const std::string& suffix) {
    
    std::vector<KeywordUsageInfo> usages;
    usages.reserve(keywords.size());
    
    std::vector<std::string> types = getShaderTypes();
    
    // For each keyword, look for variable declarations
    for (const auto& keyword : keywords) {
        // Look for patterns like: <type> <keyword> = ...;
        for (const auto& type : types) {
            size_t pos = 0;
            while ((pos = source.find(type, pos)) != std::string::npos) {
                // Check if type is a whole word
                if (pos > 0 && (std::isalnum(source[pos - 1]) || source[pos - 1] == '_')) {
                    pos += type.length();
                    continue;
                }
                
                size_t afterType = pos + type.length();
                if (afterType < source.length() && (std::isalnum(source[afterType]) || source[afterType] == '_')) {
                    pos += type.length();
                    continue;
                }
                
                // Skip whitespace
                while (afterType < source.length() && std::isspace(source[afterType])) {
                    ++afterType;
                }
                
                // Check if next word is our keyword
                if (afterType + keyword.length() <= source.length() &&
                    source.substr(afterType, keyword.length()) == keyword) {
                    
                    size_t afterKeyword = afterType + keyword.length();
                    
                    // Make sure keyword is a whole word
                    if (afterKeyword < source.length() && (std::isalnum(source[afterKeyword]) || source[afterKeyword] == '_')) {
                        pos += type.length();
                        continue;
                    }
                    
                    // Skip whitespace
                    while (afterKeyword < source.length() && std::isspace(source[afterKeyword])) {
                        ++afterKeyword;
                    }
                    
                    // Check for = sign (variable declaration)
                    if (afterKeyword < source.length() && source[afterKeyword] == '=') {
                        // Find the semicolon
                        size_t semicolon = source.find(';', afterKeyword);
                        if (semicolon != std::string::npos) {
                            KeywordUsageInfo info;
                            info.keyword = keyword;
                            info.newName = keyword + suffix;
                            info.declarationStart = pos;
                            info.declarationEnd = semicolon + 1;
                            
                            usages.push_back(std::move(info));
                            
                            pos = semicolon + 1;
                            continue;
                        }
                    }
                }
                
                pos += type.length();
            }
        }
    }
    
    return usages;
}

std::vector<ShaderPreprocessor::ArrayInitializerInfo> ShaderPreprocessor::detectArrayInitializers(const std::string& source) {
    std::vector<ArrayInitializerInfo> arrayInits;
    
    // Pattern to match: [static] [const] <type> <name>[<size>] = <type>[<size>]{...};
    std::vector<std::string> types = getShaderTypes();
    
    size_t pos = 0;
    while (pos < source.length()) {
        // Look for array declarations with initializers
        // Start by finding the equals sign followed by brace initializer
        size_t equalsPos = source.find('=', pos);
        if (equalsPos == std::string::npos) break;
        
        // Look for opening brace after equals
        size_t searchPos = equalsPos + 1;
        while (searchPos < source.length() && std::isspace(source[searchPos])) {
            ++searchPos;
        }
        
        // Check if we have array initializer syntax: type[size]{
        bool hasArrayInit = false;
        size_t bracePos = std::string::npos;
        
        // Look for pattern: type[size]{
        size_t checkPos = searchPos;
        while (checkPos < source.length()) {
            if (source[checkPos] == '{') {
                bracePos = checkPos;
                hasArrayInit = true;
                break;
            } else if (source[checkPos] == ';' || source[checkPos] == '\n') {
                break;
            }
            ++checkPos;
        }
        
        if (!hasArrayInit || bracePos == std::string::npos) {
            pos = equalsPos + 1;
            continue;
        }
        
        // Find the matching closing brace
        int braceDepth = 1;
        size_t closeBracePos = bracePos + 1;
        while (closeBracePos < source.length() && braceDepth > 0) {
            if (source[closeBracePos] == '{') ++braceDepth;
            else if (source[closeBracePos] == '}') --braceDepth;
            ++closeBracePos;
        }
        
        if (braceDepth != 0) {
            pos = equalsPos + 1;
            continue;
        }
        
        // Find the semicolon
        size_t semicolonPos = closeBracePos;
        while (semicolonPos < source.length() && std::isspace(source[semicolonPos])) {
            ++semicolonPos;
        }
        if (semicolonPos >= source.length() || source[semicolonPos] != ';') {
            pos = equalsPos + 1;
            continue;
        }
        
        // Now work backwards to find the declaration
        // Format: [static] [const] type name[size]
        size_t declStart = equalsPos;
        while (declStart > 0 && (std::isspace(source[declStart - 1]) || source[declStart - 1] == ']')) {
            --declStart;
        }
        
        // Find the array size
        size_t closeBracketPos = declStart;
        if (closeBracketPos > 0 && source[closeBracketPos] == ']') {
            // Already at ]
        } else {
            // Find ]
            while (closeBracketPos < equalsPos && source[closeBracketPos] != ']') {
                ++closeBracketPos;
            }
            if (closeBracketPos >= equalsPos) {
                pos = equalsPos + 1;
                continue;
            }
        }
        
        // Find the opening bracket
        size_t openBracketPos = closeBracketPos;
        while (openBracketPos > 0 && source[openBracketPos] != '[') {
            --openBracketPos;
        }
        
        if (source[openBracketPos] != '[') {
            pos = equalsPos + 1;
            continue;
        }
        
        std::string arraySize = source.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1);
        // Trim whitespace
        arraySize.erase(0, arraySize.find_first_not_of(" \t\n\r"));
        arraySize.erase(arraySize.find_last_not_of(" \t\n\r") + 1);
        
        // Find array name
        size_t nameEnd = openBracketPos;
        while (nameEnd > 0 && std::isspace(source[nameEnd - 1])) {
            --nameEnd;
        }
        
        size_t nameStart = nameEnd;
        while (nameStart > 0 && (std::isalnum(source[nameStart - 1]) || source[nameStart - 1] == '_')) {
            --nameStart;
        }
        
        std::string arrayName = source.substr(nameStart, nameEnd - nameStart);
        
        // Find type end (before any keywords like const/static)
        size_t typeEnd = nameStart;
        while (typeEnd > 0 && std::isspace(source[typeEnd - 1])) {
            --typeEnd;
        }
        
        // Find where the actual type word ends (working backwards)
        size_t typeWordEnd = typeEnd;
        size_t typeWordStart = typeWordEnd;
        while (typeWordStart > 0 && (std::isalnum(source[typeWordStart - 1]) || source[typeWordStart - 1] == '_')) {
            --typeWordStart;
        }
        
        // Extract the type (this is the actual type like "float", not including static/const)
        std::string arrayType = source.substr(typeWordStart, typeWordEnd - typeWordStart);
        
        // Now check for const keyword (working backwards from the type)
        bool isConst = false;
        size_t beforeType = typeWordStart;
        while (beforeType > 0 && std::isspace(source[beforeType - 1])) {
            --beforeType;
        }
        
        if (beforeType >= 5) {
            std::string possibleConst = source.substr(beforeType - 5, 5);
            if (possibleConst == "const") {
                isConst = true;
                beforeType -= 5;
                // Skip whitespace before const
                while (beforeType > 0 && std::isspace(source[beforeType - 1])) {
                    --beforeType;
                }
            }
        }
        
        // Check for static keyword (before const)
        bool isStatic = false;
        if (beforeType >= 6) {
            std::string possibleStatic = source.substr(beforeType - 6, 6);
            if (possibleStatic == "static") {
                isStatic = true;
                beforeType -= 6;
            }
        }
        
        // Find declaration start
        // If we have static/const, start from before them; otherwise start from the type
        if (isStatic || isConst) {
            declStart = beforeType;
        } else {
            declStart = typeWordStart;
        }
        
        // Trim leading whitespace from the line
        while (declStart > 0 && (source[declStart - 1] == ' ' || source[declStart - 1] == '\t')) {
            --declStart;
        }
        
        // Parse initializer values
        std::vector<std::string> initValues;
        std::string initContent = source.substr(bracePos + 1, closeBracePos - bracePos - 2);
        
        // Split by commas (respecting nested parentheses and braces)
        size_t valueStart = 0;
        int depth = 0;
        for (size_t i = 0; i < initContent.length(); ++i) {
            char c = initContent[i];
            if (c == '(' || c == '{') {
                ++depth;
            } else if (c == ')' || c == '}') {
                --depth;
            } else if (c == ',' && depth == 0) {
                std::string value = initContent.substr(valueStart, i - valueStart);
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t\n\r"));
                value.erase(value.find_last_not_of(" \t\n\r") + 1);
                if (!value.empty()) {
                    initValues.push_back(value);
                }
                valueStart = i + 1;
            }
        }
        
        // Don't forget the last value
        if (valueStart < initContent.length()) {
            std::string value = initContent.substr(valueStart);
            value.erase(0, value.find_first_not_of(" \t\n\r"));
            value.erase(value.find_last_not_of(" \t\n\r") + 1);
            if (!value.empty()) {
                initValues.push_back(value);
            }
        }
        
        ArrayInitializerInfo info;
        info.arrayType = arrayType;
        info.arrayName = arrayName;
        info.arraySize = arraySize;
        info.initializerValues = initValues;
        info.declarationStart = declStart;
        info.declarationEnd = semicolonPos + 1;
        info.isStatic = isStatic;
        info.isConst = isConst;
        
        // Check if we need to expand vector types (float4, float3, float2, etc.)
        // This happens when array size doesn't match initializer count
        try {
            size_t expectedSize = std::stoul(arraySize);
            if (initValues.size() < expectedSize && initValues.size() > 0) {
                // We might need to expand vectors
                // Calculate how many components each initializer should expand to
                size_t expansionFactor = expectedSize / initValues.size();
                
                // Only expand if it's a perfect division and makes sense (2, 3, or 4 components)
                if (expectedSize % initValues.size() == 0 && 
                    (expansionFactor == 2 || expansionFactor == 3 || expansionFactor == 4)) {
                    
                    // Expand each initializer value with swizzles
                    std::vector<std::string> expandedValues;
                    const char* swizzles[] = {"x", "y", "z", "w"};
                    
                    for (const auto& value : initValues) {
                        for (size_t i = 0; i < expansionFactor; ++i) {
                            // Check if the value already has a swizzle or is a simple scalar
                            // For now, assume it needs expansion and add .x, .y, .z, .w
                            std::string expandedValue = "(" + value + ")." + swizzles[i];
                            expandedValues.push_back(expandedValue);
                        }
                    }
                    
                    info.initializerValues = expandedValues;
                }
            }
        } catch (...) {
            // If we can't parse the array size, proceed with original values
        }
        
        arrayInits.push_back(info);
        
        pos = semicolonPos + 1;
    }
    
    return arrayInits;
}

std::string ShaderPreprocessor::fixArrayInitializers(const std::string& shaderSource) {
    std::vector<ArrayInitializerInfo> arrayInits = detectArrayInitializers(shaderSource);
    
    if (arrayInits.empty()) {
        return shaderSource;
    }
    
    std::string result = shaderSource;
    
    // We need to:
    // 1. Replace the array declaration with a simple declaration (no static/const, no initializer)
    // 2. Find "void PS(" function and insert assignments after the opening brace
    
    // First, collect all the assignments we need to make
    std::string allAssignments;
    
    // Process in reverse order to maintain correct positions
    for (auto it = arrayInits.rbegin(); it != arrayInits.rend(); ++it) {
        const auto& info = *it;
        
        // Create the new simple declaration (without static/const and initializer)
        std::string newDeclaration = info.arrayType + " " + info.arrayName + "[" + info.arraySize + "];\n";
        
        // Create assignments for each initializer value
        std::string assignments;
        for (size_t i = 0; i < info.initializerValues.size(); ++i) {
            assignments += info.arrayName + "[" + std::to_string(i) + "]=" + info.initializerValues[i] + ";\n";
        }
        
        // Prepend to collected assignments (since we're processing in reverse)
        allAssignments = assignments + allAssignments;
        
        // Replace the declaration in the source
        result.replace(info.declarationStart, info.declarationEnd - info.declarationStart, newDeclaration);
    }
    
    // Now find "void PS(" function and insert assignments after its opening brace
    // Pattern: void PS(...) { or void PS(...)\n{
    size_t psFuncPos = result.find("void PS(");
    if (psFuncPos != std::string::npos) {
        // Find the opening brace after the function signature
        size_t searchPos = psFuncPos + 8; // Skip "void PS("
        
        // Find the closing parenthesis of the parameter list
        int parenDepth = 1;
        while (searchPos < result.length() && parenDepth > 0) {
            if (result[searchPos] == '(') {
                ++parenDepth;
            } else if (result[searchPos] == ')') {
                --parenDepth;
            }
            ++searchPos;
        }
        
        // Now find the opening brace
        while (searchPos < result.length() && result[searchPos] != '{') {
            ++searchPos;
        }
        
        if (searchPos < result.length() && result[searchPos] == '{') {
            // Insert assignments right after the opening brace
            result.insert(searchPos + 1, "\n" + allAssignments);
        }
    }
    
    return result;
}

std::string ShaderPreprocessor::fixModuloParentheses(const std::string& shaderSource) {
    std::string result = shaderSource;
    
    // Look for patterns like: identifier%number/something or identifier%number*something
    // where the modulo operation should be parenthesized
    
    bool foundIssue = true;
    int maxIterations = 100;
    int iteration = 0;
    
    while (foundIssue && iteration < maxIterations) {
        foundIssue = false;
        iteration++;
        
        // Create cleaned version to avoid issues with comments
        std::string cleaned = removeCommentsAndStrings(result);
        
        if (cleaned.length() != result.length()) {
            if (m_verbose) {
                std::cout << "Error: length mismatch in fixModuloParentheses!" << std::endl;
            }
            break;
        }
        
        for (size_t i = 0; i < cleaned.length(); ++i) {
            if (cleaned[i] != '%') {
                continue;
            }
            
            // Found modulo operator, check if it needs parentheses
            // Look backwards for the left operand
            size_t leftEnd = i;
            if (leftEnd == 0) continue;
            
            // Skip back past any whitespace
            size_t leftPos = leftEnd - 1;
            while (leftPos > 0 && std::isspace(cleaned[leftPos])) {
                --leftPos;
            }
            
            // If we hit a closing paren, this expression is already parenthesized
            if (cleaned[leftPos] == ')') {
                continue;
            }
            
            // Find the start of the left operand (identifier or number)
            size_t leftStart = leftPos;
            while (leftStart > 0 && (std::isalnum(cleaned[leftStart - 1]) || cleaned[leftStart - 1] == '_' || cleaned[leftStart - 1] == '.')) {
                --leftStart;
            }
            
            // Look forward for the right operand
            size_t rightStart = i + 1;
            while (rightStart < cleaned.length() && std::isspace(cleaned[rightStart])) {
                ++rightStart;
            }
            
            if (rightStart >= cleaned.length()) {
                continue;
            }
            
            // Find the end of the right operand
            size_t rightEnd = rightStart;
            while (rightEnd < cleaned.length() && (std::isalnum(cleaned[rightEnd]) || cleaned[rightEnd] == '_' || cleaned[rightEnd] == '.')) {
                ++rightEnd;
            }
            
            if (rightEnd >= cleaned.length()) {
                continue;
            }
            
            // Skip whitespace after right operand
            size_t afterRight = rightEnd;
            while (afterRight < cleaned.length() && std::isspace(cleaned[afterRight])) {
                ++afterRight;
            }
            
            if (afterRight >= cleaned.length()) {
                continue;
            }
            
            // Check if the next operator is / or * (lower precedence than %)
            // In these cases, we need to add parentheses
            char nextOp = cleaned[afterRight];
            if (nextOp == '/' || nextOp == '*') {
                // Check what comes before the left operand
                // We want to add parentheses only if it's not already parenthesized
                char beforeChar = (leftStart > 0) ? cleaned[leftStart - 1] : ' ';
                
                // Don't add parentheses if already inside parentheses or at start of expression
                if (beforeChar == '(') {
                    continue;
                }
                
                if (m_verbose) {
                    std::cout << "Found unparenthesized modulo at position " << i << std::endl;
                    if (leftStart >= 5 && rightEnd + 10 < result.length()) {
                        std::cout << "  Context: " << result.substr(leftStart - 5, rightEnd - leftStart + 15) << std::endl;
                    }
                }
                
                // Add closing parenthesis after right operand
                result.insert(rightEnd, ")");
                
                // Add opening parenthesis before left operand
                result.insert(leftStart, "(");
                
                foundIssue = true;
                
                if (m_verbose) {
                    std::cout << "  Added parentheses around modulo operation" << std::endl;
                    if (leftStart >= 5 && rightEnd + 10 < result.length()) {
                        std::cout << "  Result: " << result.substr(leftStart - 5, rightEnd - leftStart + 17) << std::endl;
                    }
                }
                
                // Start over after making a change
                break;
            }
        }
    }
    
    if (m_verbose && iteration >= maxIterations) {
        std::cout << "Warning: fixModuloParentheses reached max iterations" << std::endl;
    }
    
    return result;
}

void ShaderPreprocessor::removeEmptyLines(std::string& str) {
    auto newEnd = std::unique(str.begin(), str.end(),
        [](char a, char b) {
            return (a == '\n' && b == '\n');
        });
    str.erase(newEnd, str.end());
}

std::string ShaderPreprocessor::preprocess(const std::string& shaderSource) {
    std::string result = shaderSource;

    // Step 0: Process #define directives (expand macros and remove directives)
    result = processDefines(result);


    // Step 1: Fix array initializers (must be done early, before shader_body transformations)
//    if (m_language == ShaderLanguage::HLSL) {
//        result = fixArrayInitializers(result);
//    }

    // Step 2: Remove invalid functions
    std::vector<FunctionInfo> functions = extractFunctions(result);
    for (auto it = functions.rbegin(); it != functions.rend(); ++it) {
        if (it->shouldReturnValue && !it->hasReturn) {
            result.erase(it->startPos, it->length);
        }
    }

    // Step 3: Fix variable shadowing
    std::vector<ShadowingInfo> shadowingCases = detectShadowing(result);
    
    if (!shadowingCases.empty()) {
        int offset = 0;
        
        for (const auto& shadow : shadowingCases) {
            std::string newVarName = shadow.varName + "_" + shadow.varType;
            
            size_t declStart = shadow.declarationStart + offset;
            size_t declEnd = shadow.declarationEnd + offset;
            
            std::string declaration = result.substr(declStart, declEnd - declStart);
            
            size_t typePos = declaration.find(shadow.varType);
            if (typePos != std::string::npos) {
                size_t varPos = typePos + shadow.varType.length();
                
                while (varPos < declaration.length() && std::isspace(declaration[varPos])) {
                    ++varPos;
                }
                
                if (varPos < declaration.length() &&
                    declaration.substr(varPos, shadow.varName.length()) == shadow.varName) {
                    declaration.replace(varPos, shadow.varName.length(), newVarName);
                }
            }
            
            result.replace(declStart, declEnd - declStart, declaration);
            
            size_t searchStart = declStart + declaration.length();
            size_t pos = searchStart;
            
            while (pos < result.length()) {
                pos = result.find(shadow.varName, pos);
                if (pos == std::string::npos) break;
                
                bool isWordStart = (pos == 0 || !std::isalnum(result[pos - 1]) && result[pos - 1] != '_');
                bool isWordEnd = (pos + shadow.varName.length() >= result.length() ||
                                 !std::isalnum(result[pos + shadow.varName.length()]) &&
                                 result[pos + shadow.varName.length()] != '_');
                
                if (isWordStart && isWordEnd) {
                    result.replace(pos, shadow.varName.length(), newVarName);
                    pos += newVarName.length();
                } else {
                    pos += shadow.varName.length();
                }
            }
            
            offset += static_cast<int>(newVarName.length()) - static_cast<int>(shadow.varName.length());
        }
    }
    
    // Step 4: Fix division by zero
    std::vector<ForLoopInfo> dangerousLoops = detectDivisionByZeroInLoops(result);
    
    if (!dangerousLoops.empty()) {
        int offset = 0;
        
        for (const auto& loop : dangerousLoops) {
            size_t forStart = loop.forStatementStart + offset;
            size_t forEnd = loop.forStatementEnd + offset;
            
            std::string forStatement = result.substr(forStart, forEnd - forStart);
            
            size_t varPos = forStatement.find(loop.loopVariable);
            if (varPos != std::string::npos) {
                size_t equalPos = forStatement.find('=', varPos);
                if (equalPos != std::string::npos) {
                    size_t zeroPos = equalPos + 1;
                    
                    while (zeroPos < forStatement.length() && std::isspace(forStatement[zeroPos])) {
                        ++zeroPos;
                    }
                    
                    if (zeroPos < forStatement.length() && forStatement[zeroPos] == '0') {
                        if (zeroPos + 1 >= forStatement.length() ||
                            !std::isalnum(forStatement[zeroPos + 1])) {
                            forStatement[zeroPos] = '1';
                        }
                    }
                }
            }
            
            result.replace(forStart, forEnd - forStart, forStatement);
        }
    }
    
    // Step 4.5: Remove redundant parentheses
    result = removeRedundantParentheses(result);

    // Step 4.6: Add missing parentheses around modulo operations
    result = fixModuloParentheses(result);
    
    // Step 5: Clean preprocessor directives
//    size_t pos = 0;
//    while (pos < result.length()) {
//        if (pos == 0 || result[pos - 1] == '\n') {
//            if (result[pos] == '#') {
//                size_t spaceStart = pos + 1;
//                size_t spaceEnd = spaceStart;
//                
//                while (spaceEnd < result.length() &&
//                       (result[spaceEnd] == ' ' || result[spaceEnd] == '\t')) {
//                    ++spaceEnd;
//                }
//                
//                if (spaceEnd > spaceStart) {
//                    result.erase(spaceStart, spaceEnd - spaceStart);
//                }
//                
//                pos = result.find('\n', pos);
//                if (pos == std::string::npos) break;
//            }
//        }
//        ++pos;
//    }
    
    // Step 6: Rename keywords used as variables (HLSL-specific)
    if (m_language == ShaderLanguage::HLSL) {
        std::vector<std::string> hlslKeywords = {"sample"};
        result = renameKeywordsAsVariables(result, hlslKeywords, "_var");
    }
    
    // Step 7: Fix complex for loops
    bool foundComplexLoop = true;
    int iterationCount = 0;
    
    while (foundComplexLoop) {
        std::vector<ComplexForLoopInfo> complexLoops = detectComplexForLoops(result);
        foundComplexLoop = false;
        
        if (complexLoops.empty()) {
            break;
        }
        
        iterationCount++;
        if (m_verbose) {
            std::cout << "\n=== Iteration " << iterationCount << ": Found " << complexLoops.size() << " total loops ===" << std::endl;
        }
        
        const ComplexForLoopInfo* loopToProcess = nullptr;
        int loopIndex = 0;
        
        for (const auto& loop : complexLoops) {
            loopIndex++;
            
            int parenDepth = 0;
            bool hasMultipleInit = false;
            for (char c : loop.initialization) {
                if (c == '(') ++parenDepth;
                else if (c == ')') --parenDepth;
                else if (c == ',' && parenDepth == 0) {
                    hasMultipleInit = true;
                    break;
                }
            }
            
            parenDepth = 0;
            bool hasMultipleIncrement = false;
            for (char c : loop.increment) {
                if (c == '(') ++parenDepth;
                else if (c == ')') --parenDepth;
                else if (c == ',' && parenDepth == 0) {
                    hasMultipleIncrement = true;
                    break;
                }
            }
            
            if (m_verbose) {
                std::cout << "Checking loop #" << loopIndex << " at position " << loop.forStart << std::endl;
                std::cout << "  Init: '" << (loop.initialization.length() > 50 ? loop.initialization.substr(0, 50) + "..." : loop.initialization) << "'" << std::endl;
                std::cout << "  Cond: '" << loop.condition << "'" << std::endl;
                std::cout << "  Incr: '" << (loop.increment.length() > 50 ? loop.increment.substr(0, 50) + "..." : loop.increment) << "'" << std::endl;
                std::cout << "  hasMultipleInit=" << hasMultipleInit << ", hasMultipleIncrement=" << hasMultipleIncrement << std::endl;
            }
            
            if (hasMultipleInit || hasMultipleIncrement) {
                loopToProcess = &loop;
                foundComplexLoop = true;
                if (m_verbose) {
                    std::cout << "  -> This loop IS complex, will process it" << std::endl;
                }
                break;
            } else {
                if (m_verbose) {
                    std::cout << "  -> This loop is NOT complex, skipping" << std::endl;
                }
            }
        }
        
        if (!loopToProcess) {
            if (m_verbose) {
                std::cout << "No complex loops found. Done." << std::endl;
            }
            break;
        }
        
        const auto& loop = *loopToProcess;
        
        int parenDepth = 0;
        bool hasMultipleInit = false;
        for (char c : loop.initialization) {
            if (c == '(') ++parenDepth;
            else if (c == ')') --parenDepth;
            else if (c == ',' && parenDepth == 0) {
                hasMultipleInit = true;
                break;
            }
        }
        
        parenDepth = 0;
        bool hasMultipleIncrement = false;
        for (char c : loop.increment) {
            if (c == '(') ++parenDepth;
            else if (c == ')') --parenDepth;
            else if (c == ',' && parenDepth == 0) {
                hasMultipleIncrement = true;
                break;
            }
        }
        
        std::string newInit = hasMultipleInit ? "" : loop.initialization;
        std::string newIncrement = hasMultipleIncrement ? "" : loop.increment;
        
        std::string newForHeader = "for (" + newInit + "; " + loop.condition + "; " + newIncrement + ")";
        
        std::string loopBody = result.substr(loop.bodyStart, loop.bodyEnd - loop.bodyStart);
        
        std::string newBody;
        
        if (hasMultipleIncrement) {
            // Add increment at end of loop body
            if (loop.hasBlockBody) {
                std::string innerBody = loopBody.substr(1, loopBody.length() - 2);
                newBody = "{\n" + innerBody;
                if (!innerBody.empty() && innerBody.back() != '\n') {
                    newBody += "\n";
                }
                newBody += loop.increment + ";\n}";
            } else {
                newBody = "{\n" + loopBody;
                if (!loopBody.empty() && loopBody.back() != '\n') {
                    newBody += "\n";
                }
                newBody += loop.increment + ";\n}";
            }
        } else {
            // No increment to add, keep body as-is (or wrap if single statement)
            if (loop.hasBlockBody) {
                newBody = loopBody;
            } else {
                newBody = "{\n" + loopBody + "\n}";
            }
        }
        
        std::string replacement;
        if (hasMultipleInit) {
            // Wrap everything: initialization before loop, then the loop
            replacement = "{\n" + loop.initialization + ";\n" + newForHeader + " " + newBody + "\n}";
        } else {
            replacement = newForHeader + " " + newBody;
        }
        
        result.replace(loop.forStart, loop.bodyEnd - loop.forStart, replacement);
    }
    
    // Step 7.5: Fix empty for loop initializers and increments
    result = fixEmptyForLoopParts(result);

    // Step 8: Apply HLSLTypeFixer as final step (HLSL-specific)
    if (m_language == ShaderLanguage::HLSL) {
        HLSLTypeFixer hlslTypeFixer;
        result = hlslTypeFixer.autoFix(result);
    }
    
    removeEmptyLines(result);

    return result;
}

std::string ShaderPreprocessor::fixVariableShadowing(const std::string& shaderSource) {
    std::vector<ShadowingInfo> shadowingCases = detectShadowing(shaderSource);
    
    if (shadowingCases.empty()) {
        return shaderSource;
    }
    
    std::string result = shaderSource;
    int offset = 0;
    
    for (const auto& shadow : shadowingCases) {
        std::string newVarName = shadow.varName + "_" + shadow.varType;
        
        size_t declStart = shadow.declarationStart + offset;
        size_t declEnd = shadow.declarationEnd + offset;
        
        std::string declaration = result.substr(declStart, declEnd - declStart);
        
        std::string pattern = R"(\b)" + shadow.varType + R"(\s+)" + shadow.varName + R"(\b)";
        std::regex varRegex(pattern);
        std::string replacement = shadow.varType + " " + newVarName;
        
        std::string newDeclaration = std::regex_replace(declaration, varRegex, replacement, std::regex_constants::format_first_only);
        
        result.replace(declStart, declEnd - declStart, newDeclaration);
        
        size_t searchStart = declStart + newDeclaration.length();
        std::string remainingCode = result.substr(searchStart);
        
        std::string wordPattern = R"(\b)" + shadow.varName + R"(\b)";
        std::regex wordRegex(wordPattern);
        std::string replacedRemaining = std::regex_replace(remainingCode, wordRegex, newVarName);
        
        result.replace(searchStart, remainingCode.length(), replacedRemaining);
        
        offset += static_cast<int>(newVarName.length()) - static_cast<int>(shadow.varName.length());
    }
    
    return result;
}

std::vector<ShaderPreprocessor::ShadowingInfo> ShaderPreprocessor::detectShadowing(const std::string& source) {
    std::vector<ShadowingInfo> shadowingCases;
    shadowingCases.reserve(8);
    
    std::vector<std::string> types = getShaderTypes();
    
    // Manual parsing instead of regex for each type
    for (const auto& type : types) {
        size_t pos = 0;
        while ((pos = source.find(type, pos)) != std::string::npos) {
            // Check if it's a whole word
            if (pos > 0 && (std::isalnum(source[pos - 1]) || source[pos - 1] == '_')) {
                pos += type.length();
                continue;
            }
            
            size_t afterType = pos + type.length();
            if (afterType < source.length() && (std::isalnum(source[afterType]) || source[afterType] == '_')) {
                pos += type.length();
                continue;
            }
            
            // Skip whitespace
            while (afterType < source.length() && std::isspace(source[afterType])) {
                ++afterType;
            }
            
            // Get variable name
            size_t nameStart = afterType;
            size_t nameEnd = nameStart;
            while (nameEnd < source.length() && (std::isalnum(source[nameEnd]) || source[nameEnd] == '_')) {
                ++nameEnd;
            }
            
            if (nameStart >= nameEnd) {
                pos += type.length();
                continue;
            }
            
            std::string varName = source.substr(nameStart, nameEnd - nameStart);
            
            // Skip whitespace
            size_t afterName = nameEnd;
            while (afterName < source.length() && std::isspace(source[afterName])) {
                ++afterName;
            }
            
            // Check for = sign
            if (afterName >= source.length() || source[afterName] != '=') {
                pos = nameEnd;
                continue;
            }
            
            // Find the semicolon
            size_t semicolon = source.find(';', afterName);
            if (semicolon == std::string::npos) {
                pos = nameEnd;
                continue;
            }
            
            // Check if variable name appears in initialization with . or [
            std::string initExpr = source.substr(afterName + 1, semicolon - afterName - 1);
            size_t varUse = initExpr.find(varName);
            if (varUse != std::string::npos) {
                // Check if followed by . or [
                size_t afterVar = varUse + varName.length();
                if (afterVar < initExpr.length() && (initExpr[afterVar] == '.' || initExpr[afterVar] == '[')) {
                    ShadowingInfo info;
                    info.varType = type;
                    info.varName = varName;
                    info.originalName = varName;
                    info.declarationStart = pos;
                    info.declarationEnd = semicolon + 1;
                    
                    shadowingCases.push_back(std::move(info));
                }
            }
            
            pos = semicolon + 1;
        }
    }
    
    return shadowingCases;
}

std::string ShaderPreprocessor::fixDivisionByZero(const std::string& shaderSource) {
    std::vector<ForLoopInfo> dangerousLoops = detectDivisionByZeroInLoops(shaderSource);
    
    if (dangerousLoops.empty()) {
        return shaderSource;
    }
    
    std::string result = shaderSource;
    int offset = 0;
    
    for (const auto& loop : dangerousLoops) {
        size_t forStart = loop.forStatementStart + offset;
        size_t forEnd = loop.forStatementEnd + offset;
        
        std::string forStatement = result.substr(forStart, forEnd - forStart);
        
        std::string pattern = R"(\b)" + loop.loopVariable + R"(\s*=\s*0\b)";
        std::regex initRegex(pattern);
        std::string replacement = loop.loopVariable + " = 1";
        
        std::string newForStatement = std::regex_replace(forStatement, initRegex, replacement, std::regex_constants::format_first_only);
        
        result.replace(forStart, forEnd - forStart, newForStatement);
        
        offset += static_cast<int>(newForStatement.length()) - static_cast<int>(forStatement.length());
    }
    
    return result;
}

std::vector<ShaderPreprocessor::ForLoopInfo> ShaderPreprocessor::detectDivisionByZeroInLoops(const std::string& source) {
    std::vector<ForLoopInfo> dangerousLoops;
    dangerousLoops.reserve(4);
    
    // Manual parsing instead of regex
    size_t pos = 0;
    while ((pos = source.find("for", pos)) != std::string::npos) {
        // Check if it's a whole word
        if (pos > 0 && (std::isalnum(source[pos - 1]) || source[pos - 1] == '_')) {
            pos += 3;
            continue;
        }
        
        if (pos + 3 < source.length() && (std::isalnum(source[pos + 3]) || source[pos + 3] == '_')) {
            pos += 3;
            continue;
        }
        
        // Find opening parenthesis
        size_t openParen = pos + 3;
        while (openParen < source.length() && std::isspace(source[openParen])) ++openParen;
        
        if (openParen >= source.length() || source[openParen] != '(') {
            pos += 3;
            continue;
        }
        
        // Find the semicolon that ends the initialization part
        size_t firstSemicolon = source.find(';', openParen);
        if (firstSemicolon == std::string::npos) {
            pos += 3;
            continue;
        }
        
        // Check if initialization contains "= 0"
        std::string initPart = source.substr(openParen + 1, firstSemicolon - openParen - 1);
        size_t equalsZero = initPart.find("= 0");
        if (equalsZero == std::string::npos) {
            equalsZero = initPart.find("=0");
            if (equalsZero == std::string::npos) {
                pos += 3;
                continue;
            }
        }
        
        // Extract variable name
        size_t varEnd = equalsZero;
        while (varEnd > 0 && std::isspace(initPart[varEnd - 1])) --varEnd;
        size_t varStart = varEnd;
        while (varStart > 0 && (std::isalnum(initPart[varStart - 1]) || initPart[varStart - 1] == '_')) {
            --varStart;
        }
        
        if (varStart >= varEnd) {
            pos += 3;
            continue;
        }
        
        std::string loopVar = initPart.substr(varStart, varEnd - varStart);
        
        // Find closing parenthesis
        int parenDepth = 1;
        size_t closeParen = openParen + 1;
        while (closeParen < source.length() && parenDepth > 0) {
            if (source[closeParen] == '(') ++parenDepth;
            else if (source[closeParen] == ')') --parenDepth;
            ++closeParen;
        }
        
        if (parenDepth != 0) {
            pos += 3;
            continue;
        }
        
        // Find loop body
        auto bodyOpt = findLoopBody(source, closeParen);
        if (!bodyOpt.has_value()) {
            pos = closeParen;
            continue;
        }
        
        auto [bodyStart, bodyEnd] = bodyOpt.value();
        
        // Check if loop variable is used in division
        std::string loopBody = source.substr(bodyStart, bodyEnd - bodyStart);
        size_t divPos = loopBody.find('/');
        while (divPos != std::string::npos) {
            // Check if followed by loop variable
            size_t afterDiv = divPos + 1;
            while (afterDiv < loopBody.length() && std::isspace(loopBody[afterDiv])) ++afterDiv;
            
            if (afterDiv + loopVar.length() <= loopBody.length() &&
                loopBody.substr(afterDiv, loopVar.length()) == loopVar) {
                // Check it's a whole word
                size_t afterVar = afterDiv + loopVar.length();
                if (afterVar >= loopBody.length() || (!std::isalnum(loopBody[afterVar]) && loopBody[afterVar] != '_')) {
                    ForLoopInfo info;
                    info.loopVariable = loopVar;
                    info.forStatementStart = pos;
                    info.forStatementEnd = closeParen;
                    info.loopBodyStart = bodyStart;
                    info.loopBodyEnd = bodyEnd;
                    info.initValue = "0";
                    
                    dangerousLoops.push_back(std::move(info));
                    break;
                }
            }
            
            divPos = loopBody.find('/', divPos + 1);
        }
        
        pos = bodyEnd;
    }
    
    return dangerousLoops;
}

std::optional<std::pair<size_t, size_t>> ShaderPreprocessor::findLoopBody(const std::string& source, size_t forEnd) const {
    size_t pos = forEnd;
    while (pos < source.length() && (std::isspace(source[pos]) || source[pos] == '\n')) {
        ++pos;
    }
    
    if (pos >= source.length()) {
        return std::nullopt;
    }
    
    if (source[pos] == '{') {
        auto endOpt = findMatchingBrace(source, pos);
        if (!endOpt.has_value()) {
            return std::nullopt;
        }
        return std::make_pair(pos, endOpt.value() + 1);
    } else {
        if (pos + 3 <= source.length() && source.substr(pos, 3) == "for") {
            auto nestedLoop = parseForLoopHeader(source, pos);
            if (nestedLoop.has_value()) {
                return std::make_pair(pos, nestedLoop.value().bodyEnd);
            }
        }
        
        if (pos + 2 <= source.length() && source.substr(pos, 2) == "if") {
            size_t parenPos = source.find('(', pos);
            if (parenPos != std::string::npos) {
                auto parenEnd = findMatchingBrace(source, parenPos);
                if (parenEnd.has_value()) {
                    auto ifBody = findLoopBody(source, parenEnd.value() + 1);
                    if (ifBody.has_value()) {
                        return std::make_pair(pos, ifBody.value().second);
                    }
                }
            }
        }
        
        size_t endPos = source.find(';', pos);
        if (endPos == std::string::npos) {
            return std::nullopt;
        }
        return std::make_pair(pos, endPos + 1);
    }
}

std::string ShaderPreprocessor::fixComplexForLoops(const std::string& shaderSource) {
    std::vector<ComplexForLoopInfo> complexLoops = detectComplexForLoops(shaderSource);
    
    if (complexLoops.empty()) {
        if (m_verbose) {
            std::cout << "No complex loops found." << std::endl;
        }
        return shaderSource;
    }
    
    std::string result = shaderSource;
    
    for (auto it = complexLoops.rbegin(); it != complexLoops.rend(); ++it) {
        const auto& loop = *it;
        
        int parenDepth = 0;
        bool hasMultipleInit = false;
        for (char c : loop.initialization) {
            if (c == '(') ++parenDepth;
            else if (c == ')') --parenDepth;
            else if (c == ',' && parenDepth == 0) {
                hasMultipleInit = true;
                break;
            }
        }
        
        parenDepth = 0;
        bool hasMultipleIncrement = false;
        for (char c : loop.increment) {
            if (c == '(') ++parenDepth;
            else if (c == ')') --parenDepth;
            else if (c == ',' && parenDepth == 0) {
                hasMultipleIncrement = true;
                break;
            }
        }
        
        if (!hasMultipleInit && !hasMultipleIncrement) {
            if (m_verbose) {
                std::cout << "Loop at position " << loop.forStart << " is not complex, skipping." << std::endl;
            }
            continue;
        }
        
        if (m_verbose) {
            std::cout << "Processing loop at position " << loop.forStart << std::endl;
            std::cout << "  hasMultipleInit=" << hasMultipleInit << ", hasMultipleIncrement=" << hasMultipleIncrement << std::endl;
        }
        
        std::string newInit = hasMultipleInit ? "" : loop.initialization;
        std::string newIncrement = hasMultipleIncrement ? "" : loop.increment;
        
        std::string newForHeader = "for (" + newInit + "; " + loop.condition + "; " + newIncrement + ")";
        
        std::string loopBody = result.substr(loop.bodyStart, loop.bodyEnd - loop.bodyStart);
        
        std::string newBody;
        std::string bodyPrefix;
        std::string bodySuffix;
        
        if (hasMultipleInit) {
            bodyPrefix = loop.initialization + ";\n";
        }
        
        if (hasMultipleIncrement) {
            bodySuffix = loop.increment + ";\n";
        }
        
        if (loop.hasBlockBody) {
            std::string innerBody = loopBody.substr(1, loopBody.length() - 2);
            newBody = "{\n" + bodyPrefix + innerBody;
            if (!innerBody.empty() && innerBody.back() != '\n') {
                newBody += "\n";
            }
            newBody += bodySuffix + "}";
        } else {
            newBody = "{\n" + bodyPrefix + loopBody;
            if (!loopBody.empty() && loopBody.back() != '\n') {
                newBody += "\n";
            }
            newBody += bodySuffix + "}";
        }
        
        std::string replacement;
        if (hasMultipleInit) {
            replacement = "{\n" + loop.initialization + ";\n" + newForHeader + " " + newBody + "\n}";
        } else {
            replacement = newForHeader + " " + newBody;
        }
        
        result.replace(loop.forStart, loop.bodyEnd - loop.forStart, replacement);
    }
    
    return result;
}

std::vector<ShaderPreprocessor::ComplexForLoopInfo> ShaderPreprocessor::detectComplexForLoops(const std::string& source) {
    std::vector<ComplexForLoopInfo> complexLoops;
    
    size_t pos = 0;
    int loopCount = 0;
    while (pos < source.length()) {
        size_t forPos = source.find("for", pos);
        if (forPos == std::string::npos) break;
        
        bool isWordStart = (forPos == 0 || (!std::isalnum(source[forPos - 1]) && source[forPos - 1] != '_'));
        bool isWordEnd = (forPos + 3 >= source.length() || (!std::isalnum(source[forPos + 3]) && source[forPos + 3] != '_'));
        
        if (isWordStart && isWordEnd) {
            if (m_verbose) {
                std::cout << "  Found 'for' keyword at position " << forPos << std::endl;
            }
            
            auto loopOpt = parseForLoopHeader(source, forPos);
            if (loopOpt.has_value()) {
                loopCount++;
                if (m_verbose) {
                    std::cout << "    Successfully parsed loop #" << loopCount << std::endl;
                }
                complexLoops.push_back(loopOpt.value());
                pos = loopOpt.value().headerEnd;
                continue;
            } else {
                if (m_verbose) {
                    std::cout << "    Failed to parse loop header" << std::endl;
                }
            }
        }
        
        pos = forPos + 3;
    }
    
    if (m_verbose) {
        std::cout << "  Total loops detected: " << complexLoops.size() << std::endl;
    }
    
    return complexLoops;
}

std::optional<ShaderPreprocessor::ComplexForLoopInfo> ShaderPreprocessor::parseForLoopHeader(
    const std::string& source, size_t forPos) const {
    
    ComplexForLoopInfo info;
    info.forStart = forPos;
    
    size_t pos = forPos + 3;
    while (pos < source.length() && std::isspace(source[pos])) ++pos;
    
    if (pos >= source.length() || source[pos] != '(') {
        return std::nullopt;
    }
    
    size_t headerStart = pos + 1;
    
    int parenDepth = 1;
    ++pos;
    while (pos < source.length() && parenDepth > 0) {
        if (source[pos] == '(') ++parenDepth;
        else if (source[pos] == ')') --parenDepth;
        ++pos;
    }
    
    if (parenDepth != 0) {
        return std::nullopt;
    }
    
    size_t headerEnd = pos - 1;
    info.headerEnd = pos;
    
    std::string header = source.substr(headerStart, headerEnd - headerStart);
    
    std::vector<std::string> parts;
    std::string current;
    parenDepth = 0;
    
    for (char c : header) {
        if (c == '(') ++parenDepth;
        else if (c == ')') --parenDepth;
        else if (c == ';' && parenDepth == 0) {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current += c;
    }
    parts.push_back(current);
    
    if (parts.size() != 3) {
        return std::nullopt;
    }
    
    info.initialization = parts[0];
    info.condition = parts[1];
    info.increment = parts[2];
    
    auto trim = [](std::string& s) {
        size_t start = 0;
        while (start < s.length() && std::isspace(s[start])) ++start;
        size_t end = s.length();
        while (end > start && std::isspace(s[end - 1])) --end;
        s = s.substr(start, end - start);
    };
    
    trim(info.initialization);
    trim(info.condition);
    trim(info.increment);
    
    auto bodyOpt = findLoopBody(source, info.headerEnd);
    if (!bodyOpt.has_value()) {
        return std::nullopt;
    }
    
    auto [bodyStart, bodyEnd] = bodyOpt.value();
    info.bodyStart = bodyStart;
    info.bodyEnd = bodyEnd;
    info.hasBlockBody = (source[bodyStart] == '{');
    info.forEnd = info.headerEnd;
    
    return info;
}

std::vector<std::string> ShaderPreprocessor::getShaderTypes() const {
    // Cache types to avoid recreating vector every call
    if (!m_cachedTypes.empty() && m_cachedTypesLanguage == m_language) {
        return m_cachedTypes;
    }
    
    m_cachedTypesLanguage = m_language;
    
    if (m_language == ShaderLanguage::GLSL) {
        m_cachedTypes = {
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
    } else {
        m_cachedTypes = {
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
    
    return m_cachedTypes;
}

std::string ShaderPreprocessor::removeRedundantParentheses(const std::string& shaderSource) {
    std::string result = shaderSource;
    
    // We'll look for patterns like (function_name(...)) where the outer parentheses are redundant
    // This is done by identifying opening parentheses followed by an identifier and another opening paren
    
    bool foundRedundancy = true;
    int maxIterations = 100; // Prevent infinite loops
    int iteration = 0;
    
    while (foundRedundancy && iteration < maxIterations) {
        foundRedundancy = false;
        iteration++;
        
        // Create cleaned version each iteration since result changes
        std::string cleaned = removeCommentsAndStrings(result);
        
        // Safety check: lengths must match
        if (cleaned.length() != result.length()) {
            if (m_verbose) {
                std::cout << "Error: cleaned (" << cleaned.length() << ") and result (" 
                          << result.length() << ") length mismatch!" << std::endl;
            }
            break;
        }
        
        for (size_t i = 0; i < cleaned.length(); ++i) {
            if (cleaned[i] != '(') {
                continue;
            }
            
            // Found opening paren, check what follows
            size_t pos = i + 1;
            
            // Skip whitespace
            while (pos < cleaned.length() && std::isspace(cleaned[pos])) {
                ++pos;
            }
            
            if (pos >= cleaned.length()) {
                break;
            }
            
            // If the next character is another opening paren, skip this
            // This avoids patterns like (( ... )) which are more complex
            if (cleaned[pos] == '(') {
                if (m_verbose) {
                    std::cout << "  Skipping: found nested '(' at position " << pos << std::endl;
                }
                continue;
            }
            
            // Check if we have an identifier (function name or type cast)
            size_t identStart = pos;
            while (pos < cleaned.length() && (std::isalnum(cleaned[pos]) || cleaned[pos] == '_')) {
                ++pos;
            }
            
            if (pos == identStart) {
                // No identifier found
                continue;
            }
            
            std::string identifier = cleaned.substr(identStart, pos - identStart);
            
            // Skip whitespace after identifier
            while (pos < cleaned.length() && std::isspace(cleaned[pos])) {
                ++pos;
            }
            
            if (pos >= cleaned.length() || cleaned[pos] != '(') {
                // Not a function call or cast
                continue;
            }
            
            // Found function call/cast, now find its matching closing paren
            int depth = 1;
            size_t funcStart = pos;
            ++pos;
            
            while (pos < cleaned.length() && depth > 0) {
                if (cleaned[pos] == '(') {
                    ++depth;
                } else if (cleaned[pos] == ')') {
                    --depth;
                }
                ++pos;
            }
            
            if (depth != 0) {
                // Unbalanced parentheses
                continue;
            }
            
            size_t funcEnd = pos - 1; // Position of the function's closing paren
            
            // Skip whitespace after function call
            while (pos < cleaned.length() && std::isspace(cleaned[pos])) {
                ++pos;
            }
            
            if (pos >= cleaned.length() || cleaned[pos] != ')') {
                // No matching outer closing paren
                continue;
            }
            
            // Now check if the outer parentheses are redundant
            // They are redundant if:
            // 1. They wrap a single function call/cast
            // 2. The outer closing paren directly follows the function closing paren (with optional whitespace)
            
            // Check what comes before the outer opening paren
            bool isRedundant = false;
            
            if (i > 0) {
                size_t beforePos = i - 1;
                while (beforePos > 0 && std::isspace(cleaned[beforePos])) {
                    --beforePos;
                }
                
                char beforeChar = cleaned[beforePos];
                
                // Redundant if preceded by operators like =, +, -, *, /, %, &, |, ^, <, >, !, etc.
                // or by opening paren, comma, semicolon
                if (beforeChar == '=' || beforeChar == '+' || beforeChar == '-' || 
                    beforeChar == '*' || beforeChar == '/' || beforeChar == '%' ||
                    beforeChar == '&' || beforeChar == '|' || beforeChar == '^' ||
                    beforeChar == '<' || beforeChar == '>' || beforeChar == '!' ||
                    beforeChar == '(' || beforeChar == ',' || beforeChar == ';' ||
                    beforeChar == '{' || beforeChar == '[') {
                    isRedundant = true;
                }
            } else {
                // At start of file
                isRedundant = true;
            }
            
            if (isRedundant) {
                if (m_verbose) {
                    std::cout << "Found redundant parentheses around: " << identifier << "() at positions " 
                              << i << " and " << pos << std::endl;
                    if (i >= 5 && i + 25 < result.length()) {
                        std::cout << "  Before: " << result.substr(i-5, 25) << std::endl;
                    }
                }
                
                // Verify the characters at these positions are actually parentheses
                if (pos >= result.length() || result[pos] != ')') {
                    if (m_verbose) {
                        std::cout << "  Error: Expected ')' at position " << pos << std::endl;
                    }
                    continue;
                }
                if (i >= result.length() || result[i] != '(') {
                    if (m_verbose) {
                        std::cout << "  Error: Expected '(' at position " << i << std::endl;
                    }
                    continue;
                }
                
                // Remove the redundant parentheses from the RESULT string
                // IMPORTANT: Remove from back to front to preserve indices
                // Remove outer closing paren first (at position pos)
                result.erase(pos, 1);
                
                // Then remove outer opening paren (at position i)
                result.erase(i, 1);
                
                foundRedundancy = true;
                
                if (m_verbose) {
                    if (i >= 5 && i + 23 < result.length()) {
                        std::cout << "  After:  " << result.substr(i-5, 23) << std::endl;
                    }
                    std::cout << "  Removed parentheses successfully." << std::endl;
                }
                
                // Start over from the beginning after making a change
                break;
            }
        }
    }
    
    if (m_verbose && iteration >= maxIterations) {
        std::cout << "Warning: removeRedundantParentheses reached max iterations" << std::endl;
    }
    
    return result;
}

std::vector<ShaderPreprocessor::DefineInfo> ShaderPreprocessor::detectDefines(const std::string& source) {
    std::vector<DefineInfo> defines;
    
    size_t pos = 0;
    while (pos < source.length()) {
        // Look for #define at the start of a line
        if (pos == 0 || source[pos - 1] == '\n') {
            // Skip whitespace at beginning of line
            size_t lineStart = pos;
            while (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
                ++pos;
            }
            
            // Check for #define
            if (pos + 7 <= source.length() && source.substr(pos, 7) == "#define") {
                size_t defineStart = lineStart;
                pos += 7;
                
                // Check that #define is followed by whitespace
                if (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
                    // Skip whitespace after #define
                    while (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
                        ++pos;
                    }
                    
                    // Extract macro name
                    size_t nameStart = pos;
                    while (pos < source.length() && 
                           (std::isalnum(source[pos]) || source[pos] == '_')) {
                        ++pos;
                    }
                    
                    if (pos > nameStart) {
                        std::string macroName = source.substr(nameStart, pos - nameStart);
                        
                        // Check if this is a function-like macro (has parameters)
                        bool isFunctionLike = false;
                        std::vector<std::string> parameters;
                        
                        // Check immediately after macro name (no space) for '('
                        if (pos < source.length() && source[pos] == '(') {
                            isFunctionLike = true;
                            ++pos; // Skip opening paren
                            
                            // Parse parameters
                            size_t paramStart = pos;
                            while (pos < source.length() && source[pos] != ')') {
                                if (source[pos] == ',') {
                                    std::string param = source.substr(paramStart, pos - paramStart);
                                    // Trim whitespace
                                    param.erase(0, param.find_first_not_of(" \t"));
                                    param.erase(param.find_last_not_of(" \t") + 1);
                                    if (!param.empty()) {
                                        parameters.push_back(param);
                                    }
                                    ++pos;
                                    paramStart = pos;
                                } else {
                                    ++pos;
                                }
                            }
                            
                            // Get last parameter
                            if (pos > paramStart) {
                                std::string param = source.substr(paramStart, pos - paramStart);
                                param.erase(0, param.find_first_not_of(" \t"));
                                param.erase(param.find_last_not_of(" \t") + 1);
                                if (!param.empty()) {
                                    parameters.push_back(param);
                                }
                            }
                            
                            if (pos < source.length() && source[pos] == ')') {
                                ++pos; // Skip closing paren
                            }
                        }
                        
                        // Skip whitespace after macro name (or parameter list)
                        while (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
                            ++pos;
                        }
                        
                        // Extract macro value (everything until end of line)
                        size_t valueStart = pos;
                        while (pos < source.length() && source[pos] != '\n' && source[pos] != '\r') {
                            ++pos;
                        }
                        
                        std::string macroValue = source.substr(valueStart, pos - valueStart);
                        
                        // Trim trailing whitespace from macro value
                        while (!macroValue.empty() && 
                               (macroValue.back() == ' ' || macroValue.back() == '\t')) {
                            macroValue.pop_back();
                        }
                        
                        // The directive ends at the end of line content (NOT including the newline)
                        // We want to keep the newline to preserve line structure
                        size_t defineEnd = pos;
                        
                        DefineInfo info;
                        info.macroName = macroName;
                        info.macroValue = macroValue;
                        info.directiveStart = defineStart;
                        info.directiveEnd = defineEnd;
                        info.isFunctionLike = isFunctionLike;
                        info.parameters = parameters;
                        
                        // Skip past the newline for the next iteration
                        if (pos < source.length() && source[pos] == '\r') {
                            ++pos;
                        }
                        if (pos < source.length() && source[pos] == '\n') {
                            ++pos;
                        }
                        
                        defines.push_back(std::move(info));
                        
                        continue;
                    }
                }
            }
        }
        ++pos;
    }
    
    return defines;
}

std::string ShaderPreprocessor::fixEmptyForLoopParts(const std::string& shaderSource) {
    std::string result = shaderSource;
    
    // Look for for loops with empty initialization or increment
    size_t pos = 0;
    while (pos < result.length()) {
        size_t forPos = result.find("for", pos);
        if (forPos == std::string::npos) break;
        
        // Check if it's a whole word
        bool isWordStart = (forPos == 0 || (!std::isalnum(result[forPos - 1]) && result[forPos - 1] != '_'));
        bool isWordEnd = (forPos + 3 >= result.length() || (!std::isalnum(result[forPos + 3]) && result[forPos + 3] != '_'));
        
        if (!isWordStart || !isWordEnd) {
            pos = forPos + 3;
            continue;
        }
        
        // Find opening parenthesis
        size_t openParen = forPos + 3;
        while (openParen < result.length() && std::isspace(result[openParen])) ++openParen;
        
        if (openParen >= result.length() || result[openParen] != '(') {
            pos = forPos + 3;
            continue;
        }
        
        // Find the two semicolons
        size_t firstSemicolon = std::string::npos;
        size_t secondSemicolon = std::string::npos;
        int parenDepth = 1;
        size_t searchPos = openParen + 1;
        
        while (searchPos < result.length() && parenDepth > 0) {
            if (result[searchPos] == '(') {
                ++parenDepth;
            } else if (result[searchPos] == ')') {
                --parenDepth;
                if (parenDepth == 0) {
                    break;
                }
            } else if (result[searchPos] == ';' && parenDepth == 1) {
                if (firstSemicolon == std::string::npos) {
                    firstSemicolon = searchPos;
                } else if (secondSemicolon == std::string::npos) {
                    secondSemicolon = searchPos;
                }
            }
            ++searchPos;
        }
        
        if (firstSemicolon == std::string::npos || secondSemicolon == std::string::npos || parenDepth != 0) {
            pos = forPos + 3;
            continue;
        }
        
        size_t closeParen = searchPos;
        
        // Check initialization part (between openParen and firstSemicolon)
        std::string initPart = result.substr(openParen + 1, firstSemicolon - openParen - 1);
        // Trim whitespace
        size_t initStart = initPart.find_first_not_of(" \t\n\r");
        size_t initEnd = initPart.find_last_not_of(" \t\n\r");
        bool initEmpty = (initStart == std::string::npos || initEnd == std::string::npos);
        
        // Check increment part (between secondSemicolon and closeParen)
        std::string incrPart = result.substr(secondSemicolon + 1, closeParen - secondSemicolon - 1);
        // Trim whitespace
        size_t incrStart = incrPart.find_first_not_of(" \t\n\r");
        size_t incrEnd = incrPart.find_last_not_of(" \t\n\r");
        bool incrEmpty = (incrStart == std::string::npos || incrEnd == std::string::npos);
        
        if (initEmpty || incrEmpty) {
            if (m_verbose) {
                std::cout << "Fixing empty for loop parts at position " << forPos << std::endl;
                std::cout << "  Init empty: " << (initEmpty ? "yes" : "no") << std::endl;
                std::cout << "  Incr empty: " << (incrEmpty ? "yes" : "no") << std::endl;
            }
            
            // Build the new for header
            std::string newInit = initEmpty ? "1" : initPart;
            if (!initEmpty && initStart != std::string::npos && initEnd != std::string::npos) {
                newInit = initPart.substr(initStart, initEnd - initStart + 1);
            }
            
            std::string condPart = result.substr(firstSemicolon + 1, secondSemicolon - firstSemicolon - 1);
            // Trim condition whitespace
            size_t condStart = condPart.find_first_not_of(" \t\n\r");
            size_t condEnd = condPart.find_last_not_of(" \t\n\r");
            std::string newCond = (condStart != std::string::npos && condEnd != std::string::npos) ?
                                  condPart.substr(condStart, condEnd - condStart + 1) : condPart;
            
            std::string newIncr = incrEmpty ? "1" : incrPart;
            if (!incrEmpty && incrStart != std::string::npos && incrEnd != std::string::npos) {
                newIncr = incrPart.substr(incrStart, incrEnd - incrStart + 1);
            }
            
            std::string newForHeader = "for (" + newInit + ";" + newCond + ";" + newIncr + ")";
            
            // Replace just the for header
            result.replace(forPos, closeParen + 1 - forPos, newForHeader);
            
            // Continue from after the replacement
            pos = forPos + newForHeader.length();
        } else {
            pos = closeParen + 1;
        }
    }
    
    return result;
}

std::string ShaderPreprocessor::processDefines(const std::string& shaderSource) {
    std::vector<DefineInfo> defines = detectDefines(shaderSource);
    
    if (defines.empty()) {
        return shaderSource;
    }
    
    if (m_verbose) {
        std::cout << "Found " << defines.size() << " #define directive(s)" << std::endl;
    }
    
    // First pass: Remove all #define directives in reverse order
    std::string result = shaderSource;
    for (auto it = defines.rbegin(); it != defines.rend(); ++it) {
        const auto& define = *it;
        
        if (m_verbose) {
            std::cout << "Removing #define directive: " << define.macroName;
            if (define.isFunctionLike) {
                std::cout << "(";
                for (size_t i = 0; i < define.parameters.size(); ++i) {
                    if (i > 0) std::cout << ",";
                    std::cout << define.parameters[i];
                }
                std::cout << ")";
            }
            std::cout << std::endl;
            std::cout << "  Position: " << define.directiveStart << " to " << define.directiveEnd << std::endl;
        }
        
        result.erase(define.directiveStart, define.directiveEnd - define.directiveStart);
    }
    
    // Second pass: Replace all macro occurrences
    // Process in reverse order so that if one macro name contains another, 
    // the longer one gets replaced first
    for (auto it = defines.rbegin(); it != defines.rend(); ++it) {
        const auto& define = *it;
        
        if (m_verbose) {
            std::cout << "Replacing macro: " << define.macroName;
            if (define.isFunctionLike) {
                std::cout << "(...) -> " << define.macroValue << std::endl;
            } else {
                std::cout << " -> " << define.macroValue << std::endl;
            }
        }
        
        size_t searchPos = 0;
        int replacementCount = 0;
        
        while (searchPos < result.length()) {
            size_t foundPos = result.find(define.macroName, searchPos);
            if (foundPos == std::string::npos) {
                break;
            }
            
            // Check if this is a whole word (not part of another identifier)
            bool isWordStart = (foundPos == 0 || 
                               (!std::isalnum(result[foundPos - 1]) && result[foundPos - 1] != '_'));
            bool isWordEnd = (foundPos + define.macroName.length() >= result.length() ||
                             (!std::isalnum(result[foundPos + define.macroName.length()]) && 
                              result[foundPos + define.macroName.length()] != '_'));
            
            if (isWordStart && isWordEnd) {
                // For function-like macros, we need to parse the arguments
                if (define.isFunctionLike) {
                    // Check if followed by '('
                    size_t parenPos = foundPos + define.macroName.length();
                    while (parenPos < result.length() && std::isspace(result[parenPos])) {
                        ++parenPos;
                    }
                    
                    if (parenPos >= result.length() || result[parenPos] != '(') {
                        // Not a function call, skip
                        searchPos = foundPos + define.macroName.length();
                        continue;
                    }
                    
                    // Parse arguments
                    std::vector<std::string> arguments;
                    size_t argStart = parenPos + 1;
                    size_t argPos = argStart;
                    int parenDepth = 1;
                    
                    while (argPos < result.length() && parenDepth > 0) {
                        if (result[argPos] == '(') {
                            ++parenDepth;
                        } else if (result[argPos] == ')') {
                            --parenDepth;
                            if (parenDepth == 0) {
                                // End of argument list
                                std::string arg = result.substr(argStart, argPos - argStart);
                                // Trim whitespace
                                arg.erase(0, arg.find_first_not_of(" \t"));
                                arg.erase(arg.find_last_not_of(" \t") + 1);
                                if (!arg.empty()) {
                                    arguments.push_back(arg);
                                }
                                break;
                            }
                        } else if (result[argPos] == ',' && parenDepth == 1) {
                            // End of this argument
                            std::string arg = result.substr(argStart, argPos - argStart);
                            arg.erase(0, arg.find_first_not_of(" \t"));
                            arg.erase(arg.find_last_not_of(" \t") + 1);
                            if (!arg.empty()) {
                                arguments.push_back(arg);
                            }
                            argStart = argPos + 1;
                        }
                        ++argPos;
                    }
                    
                    if (parenDepth != 0) {
                        // Unbalanced parentheses, skip
                        searchPos = foundPos + define.macroName.length();
                        continue;
                    }
                    
                    // Expand the macro with the arguments
                    std::string expandedValue = define.macroValue;
                    
                    // Replace each parameter with its corresponding argument
                    for (size_t i = 0; i < define.parameters.size() && i < arguments.size(); ++i) {
                        const std::string& param = define.parameters[i];
                        const std::string& arg = arguments[i];
                        
                        // Replace all occurrences of this parameter in the expanded value
                        size_t paramPos = 0;
                        while (paramPos < expandedValue.length()) {
                            paramPos = expandedValue.find(param, paramPos);
                            if (paramPos == std::string::npos) {
                                break;
                            }
                            
                            // Check if it's a whole word
                            bool paramWordStart = (paramPos == 0 || 
                                                   (!std::isalnum(expandedValue[paramPos - 1]) && 
                                                    expandedValue[paramPos - 1] != '_'));
                            bool paramWordEnd = (paramPos + param.length() >= expandedValue.length() ||
                                                (!std::isalnum(expandedValue[paramPos + param.length()]) && 
                                                 expandedValue[paramPos + param.length()] != '_'));
                            
                            if (paramWordStart && paramWordEnd) {
                                expandedValue.replace(paramPos, param.length(), arg);
                                paramPos += arg.length();
                            } else {
                                paramPos += param.length();
                            }
                        }
                    }
                    
                    if (m_verbose) {
                        std::cout << "  Replacing at position " << foundPos 
                                  << " (function call with " << arguments.size() << " arguments)" << std::endl;
                    }
                    
                    // Replace the entire macro call with the expanded value
                    result.replace(foundPos, argPos + 1 - foundPos, expandedValue);
                    replacementCount++;
                    
                    searchPos = foundPos + expandedValue.length();
                } else {
                    // Simple text replacement
                    if (m_verbose) {
                        std::cout << "  Replacing at position " << foundPos << std::endl;
                    }
                    
                    result.replace(foundPos, define.macroName.length(), define.macroValue);
                    replacementCount++;
                    
                    // Move search position forward past the replacement
                    searchPos = foundPos + define.macroValue.length();
                }
            } else {
                searchPos = foundPos + define.macroName.length();
            }
        }
        
        if (m_verbose) {
            std::cout << "  Made " << replacementCount << " replacement(s)" << std::endl;
        }
    }
    
    return result;
}
