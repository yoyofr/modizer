//
//  ShaderPreprocessor.cpp
//  projectm-ios
//
//  Created by Yohann Magnien David on 30/10/2025.
//

#include "ShaderPreprocessor.h"
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

std::string ShaderPreprocessor::preprocess(const std::string& shaderSource) {
    std::string result = shaderSource;
    
    // Step 1: Remove invalid functions
    std::vector<FunctionInfo> functions = extractFunctions(result);
    for (auto it = functions.rbegin(); it != functions.rend(); ++it) {
        if (it->shouldReturnValue && !it->hasReturn) {
            result.erase(it->startPos, it->length);
        }
    }
    
    // Step 2: Fix variable shadowing
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
    
    // Step 3: Fix division by zero
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
    
    // Step 4: Clean preprocessor directives
    size_t pos = 0;
    while (pos < result.length()) {
        if (pos == 0 || result[pos - 1] == '\n') {
            if (result[pos] == '#') {
                size_t spaceStart = pos + 1;
                size_t spaceEnd = spaceStart;
                
                while (spaceEnd < result.length() &&
                       (result[spaceEnd] == ' ' || result[spaceEnd] == '\t')) {
                    ++spaceEnd;
                }
                
                if (spaceEnd > spaceStart) {
                    result.erase(spaceStart, spaceEnd - spaceStart);
                }
                
                pos = result.find('\n', pos);
                if (pos == std::string::npos) break;
            }
        }
        ++pos;
    }
    
    // Step 5: Fix complex for loops
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
