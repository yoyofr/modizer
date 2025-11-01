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
    : m_language(language), m_verbose(false)
{
}

void ShaderPreprocessor::setLanguage(ShaderLanguage language) noexcept
{
    m_language = language;
}

void ShaderPreprocessor::setVerbose(bool verbose) noexcept
{
    m_verbose = verbose;
}

std::string ShaderPreprocessor::removeInvalidFunctions(const std::string &shaderSource)
{
    std::vector<FunctionInfo> functions = extractFunctions(shaderSource);
    std::string result = shaderSource;

    // Process functions in reverse order to maintain correct indices
    for (auto it = functions.rbegin(); it != functions.rend(); ++it)
    {
        if (it->shouldReturnValue && !it->hasReturn)
        {
            // Remove this function from the source
            result.erase(it->startPos, it->length);
        }
    }

    return result;
}

std::vector<ShaderPreprocessor::FunctionInfo> ShaderPreprocessor::extractFunctions(const std::string &source)
{
    std::vector<FunctionInfo> functions;

    // Regex to match function declarations
    std::regex funcRegex(R"((void|\w+)\s+(\w+)\s*\([^)]*\)\s*\{)");

    auto searchStart = source.cbegin();
    std::smatch match;

    while (std::regex_search(searchStart, source.cend(), match, funcRegex))
    {
        FunctionInfo func;
        func.startPos = static_cast<size_t>(match.position(0) + std::distance(source.cbegin(), searchStart));
        func.returnType = match[1].str();
        func.shouldReturnValue = (func.returnType != "void");

        size_t bodyStart = func.startPos + match.length(0);
        auto bodyEndOpt = findMatchingBrace(source, bodyStart - 1);

        if (!bodyEndOpt.has_value())
        {
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

std::optional<size_t> ShaderPreprocessor::findMatchingBrace(const std::string &source, size_t openBracePos) const
{
    int braceCount = 0;
    bool inString = false;
    bool inComment = false;
    bool inLineComment = false;

    for (size_t i = openBracePos; i < source.length(); ++i)
    {
        char c = source[i];
        char next = (i + 1 < source.length()) ? source[i + 1] : '\0';

        if (!inString && !inComment && c == '/' && next == '/')
        {
            inLineComment = true;
            ++i;
            continue;
        }

        if (inLineComment)
        {
            if (c == '\n')
            {
                inLineComment = false;
            }
            continue;
        }

        if (!inString && !inLineComment && c == '/' && next == '*')
        {
            inComment = true;
            ++i;
            continue;
        }

        if (inComment)
        {
            if (c == '*' && next == '/')
            {
                inComment = false;
                ++i;
            }
            continue;
        }

        if (c == '"' && (i == 0 || source[i - 1] != '\\'))
        {
            inString = !inString;
            continue;
        }

        if (inString)
        {
            continue;
        }

        if (c == '{')
        {
            ++braceCount;
        }
        else if (c == '}')
        {
            --braceCount;
            if (braceCount == 0)
            {
                return i;
            }
        }
    }

    return std::nullopt;
}

bool ShaderPreprocessor::checkForReturn(const std::string &body) const
{
    std::string cleanBody = removeCommentsAndStrings(body);
    std::regex returnRegex(R"(\breturn\s+[^;]+;)");
    return std::regex_search(cleanBody, returnRegex);
}

std::string ShaderPreprocessor::removeCommentsAndStrings(const std::string &source) const
{
    std::string result;
    result.reserve(source.length());

    bool inString = false;
    bool inComment = false;
    bool inLineComment = false;

    for (size_t i = 0; i < source.length(); ++i)
    {
        char c = source[i];
        char next = (i + 1 < source.length()) ? source[i + 1] : '\0';

        if (!inString && !inComment && c == '/' && next == '/')
        {
            inLineComment = true;
            result += ' ';
            ++i;
            continue;
        }

        if (inLineComment)
        {
            if (c == '\n')
            {
                inLineComment = false;
                result += '\n';
            }
            else
            {
                result += ' ';
            }
            continue;
        }

        if (!inString && !inLineComment && c == '/' && next == '*')
        {
            inComment = true;
            result += ' ';
            ++i;
            continue;
        }

        if (inComment)
        {
            if (c == '*' && next == '/')
            {
                inComment = false;
                result += ' ';
                ++i;
            }
            else
            {
                result += (c == '\n') ? '\n' : ' ';
            }
            continue;
        }

        if (c == '"' && (i == 0 || source[i - 1] != '\\'))
        {
            inString = !inString;
            result += ' ';
            continue;
        }

        if (inString)
        {
            result += ' ';
        }
        else
        {
            result += c;
        }
    }

    return result;
}

std::string ShaderPreprocessor::preprocess(const std::string &shaderSource)
{
    std::string result = shaderSource;

    // Step 1: Remove invalid functions
    std::vector<FunctionInfo> functions = extractFunctions(result);
    for (auto it = functions.rbegin(); it != functions.rend(); ++it)
    {
        if (it->shouldReturnValue && !it->hasReturn)
        {
            result.erase(it->startPos, it->length);
        }
    }

    // Step 2: Fix variable shadowing
    std::vector<ShadowingInfo> shadowingCases = detectShadowing(result);

    if (!shadowingCases.empty())
    {
        int offset = 0;

        for (const auto &shadow : shadowingCases)
        {
            std::string newVarName = shadow.varName + "_" + shadow.varType;

            size_t declStart = shadow.declarationStart + offset;
            size_t declEnd = shadow.declarationEnd + offset;

            std::string declaration = result.substr(declStart, declEnd - declStart);

            size_t typePos = declaration.find(shadow.varType);
            if (typePos != std::string::npos)
            {
                size_t varPos = typePos + shadow.varType.length();

                while (varPos < declaration.length() && std::isspace(declaration[varPos]))
                {
                    ++varPos;
                }

                if (varPos < declaration.length() &&
                    declaration.substr(varPos, shadow.varName.length()) == shadow.varName)
                {
                    declaration.replace(varPos, shadow.varName.length(), newVarName);
                }
            }

            result.replace(declStart, declEnd - declStart, declaration);

            size_t searchStart = declStart + declaration.length();
            size_t pos = searchStart;

            while (pos < result.length())
            {
                pos = result.find(shadow.varName, pos);
                if (pos == std::string::npos)
                    break;

                bool isWordStart = (pos == 0 || !std::isalnum(result[pos - 1]) && result[pos - 1] != '_');
                bool isWordEnd = (pos + shadow.varName.length() >= result.length() ||
                                  !std::isalnum(result[pos + shadow.varName.length()]) &&
                                      result[pos + shadow.varName.length()] != '_');

                if (isWordStart && isWordEnd)
                {
                    result.replace(pos, shadow.varName.length(), newVarName);
                    pos += newVarName.length();
                }
                else
                {
                    pos += shadow.varName.length();
                }
            }

            offset += static_cast<int>(newVarName.length()) - static_cast<int>(shadow.varName.length());
        }
    }

    // Step 3: Fix division by zero
    std::vector<ForLoopInfo> dangerousLoops = detectDivisionByZeroInLoops(result);

    if (!dangerousLoops.empty())
    {
        int offset = 0;

        for (const auto &loop : dangerousLoops)
        {
            size_t forStart = loop.forStatementStart + offset;
            size_t forEnd = loop.forStatementEnd + offset;

            std::string forStatement = result.substr(forStart, forEnd - forStart);

            size_t varPos = forStatement.find(loop.loopVariable);
            if (varPos != std::string::npos)
            {
                size_t equalPos = forStatement.find('=', varPos);
                if (equalPos != std::string::npos)
                {
                    size_t zeroPos = equalPos + 1;

                    while (zeroPos < forStatement.length() && std::isspace(forStatement[zeroPos]))
                    {
                        ++zeroPos;
                    }

                    if (zeroPos < forStatement.length() && forStatement[zeroPos] == '0')
                    {
                        if (zeroPos + 1 >= forStatement.length() ||
                            !std::isalnum(forStatement[zeroPos + 1]))
                        {
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
    while (pos < result.length())
    {
        if (pos == 0 || result[pos - 1] == '\n')
        {
            if (result[pos] == '#')
            {
                size_t spaceStart = pos + 1;
                size_t spaceEnd = spaceStart;

                while (spaceEnd < result.length() &&
                       (result[spaceEnd] == ' ' || result[spaceEnd] == '\t'))
                {
                    ++spaceEnd;
                }

                if (spaceEnd > spaceStart)
                {
                    result.erase(spaceStart, spaceEnd - spaceStart);
                }

                pos = result.find('\n', pos);
                if (pos == std::string::npos)
                    break;
            }
        }
        ++pos;
    }

    // Step 5: Fix complex for loops
    bool foundComplexLoop = true;
    int iterationCount = 0;

    while (foundComplexLoop)
    {
        std::vector<ComplexForLoopInfo> complexLoops = detectComplexForLoops(result);
        foundComplexLoop = false;

        if (complexLoops.empty())
        {
            break;
        }

        iterationCount++;
        if (m_verbose)
        {
            std::cout << "\n=== Iteration " << iterationCount << ": Found " << complexLoops.size() << " total loops ===" << std::endl;
        }

        const ComplexForLoopInfo *loopToProcess = nullptr;
        int loopIndex = 0;

        for (const auto &loop : complexLoops)
        {
            loopIndex++;

            int parenDepth = 0;
            bool hasMultipleInit = false;
            for (char c : loop.initialization)
            {
                if (c == '(')
                    ++parenDepth;
                else if (c == ')')
                    --parenDepth;
                else if (c == ',' && parenDepth == 0)
                {
                    hasMultipleInit = true;
                    break;
                }
            }

            parenDepth = 0;
            bool hasMultipleIncrement = false;
            for (char c : loop.increment)
            {
                if (c == '(')
                    ++parenDepth;
                else if (c == ')')
                    --parenDepth;
                else if (c == ',' && parenDepth == 0)
                {
                    hasMultipleIncrement = true;
                    break;
                }
            }

            if (m_verbose)
            {
                std::cout << "Checking loop #" << loopIndex << " at position " << loop.forStart << std::endl;
                std::cout << "  Init: '" << (loop.initialization.length() > 50 ? loop.initialization.substr(0, 50) + "..." : loop.initialization) << "'" << std::endl;
                std::cout << "  Cond: '" << loop.condition << "'" << std::endl;
                std::cout << "  Incr: '" << (loop.increment.length() > 50 ? loop.increment.substr(0, 50) + "..." : loop.increment) << "'" << std::endl;
                std::cout << "  hasMultipleInit=" << hasMultipleInit << ", hasMultipleIncrement=" << hasMultipleIncrement << std::endl;
            }

            if (hasMultipleInit || hasMultipleIncrement)
            {
                loopToProcess = &loop;
                foundComplexLoop = true;
                if (m_verbose)
                {
                    std::cout << "  -> This loop IS complex, will process it" << std::endl;
                }
                break;
            }
            else
            {
                if (m_verbose)
                {
                    std::cout << "  -> This loop is NOT complex, skipping" << std::endl;
                }
            }
        }

        if (!loopToProcess)
        {
            if (m_verbose)
            {
                std::cout << "No complex loops found. Done." << std::endl;
            }
            break;
        }

        const auto &loop = *loopToProcess;

        int parenDepth = 0;
        bool hasMultipleInit = false;
        for (char c : loop.initialization)
        {
            if (c == '(')
                ++parenDepth;
            else if (c == ')')
                --parenDepth;
            else if (c == ',' && parenDepth == 0)
            {
                hasMultipleInit = true;
                break;
            }
        }

        parenDepth = 0;
        bool hasMultipleIncrement = false;
        for (char c : loop.increment)
        {
            if (c == '(')
                ++parenDepth;
            else if (c == ')')
                --parenDepth;
            else if (c == ',' && parenDepth == 0)
            {
                hasMultipleIncrement = true;
                break;
            }
        }

        std::string newInit = hasMultipleInit ? "" : loop.initialization;
        std::string newIncrement = hasMultipleIncrement ? "" : loop.increment;

        if (newInit.length()==0) newInit+="1";
        if (newIncrement.length()==0) newIncrement+="1";
        std::string newForHeader = "for (" + newInit + "; " + loop.condition + "; " + newIncrement + ")";

        std::string loopBody = result.substr(loop.bodyStart, loop.bodyEnd - loop.bodyStart);

        std::string newBody;
        std::string bodyPrefix;
        std::string bodySuffix;

        if (hasMultipleInit)
        {
            bodyPrefix = loop.initialization + ";\n";
        }

        if (hasMultipleIncrement)
        {
            bodySuffix = loop.increment + ";\n";
        }

        if (loop.hasBlockBody)
        {
            std::string innerBody = loopBody.substr(1, loopBody.length() - 2);
            newBody = "{\n" + bodyPrefix + innerBody;
            if (!innerBody.empty() && innerBody.back() != '\n')
            {
                newBody += "\n";
            }
            newBody += bodySuffix + "}";
        }
        else
        {
            newBody = "{\n" /*+ bodyPrefix*/ + loopBody;
            if (!loopBody.empty() && loopBody.back() != '\n')
            {
                newBody += "\n";
            }
            newBody += bodySuffix + "}";
        }

        std::string replacement;
        if (hasMultipleInit)
        {
            replacement = "{\n" + loop.initialization + ";\n" + newForHeader + " " + newBody + "\n}";
        }
        else
        {
            replacement = newForHeader + " " + newBody;
        }

        result.replace(loop.forStart, loop.bodyEnd - loop.forStart, replacement);
    }

    return result;
}

std::string ShaderPreprocessor::fixVariableShadowing(const std::string &shaderSource)
{
    std::vector<ShadowingInfo> shadowingCases = detectShadowing(shaderSource);

    if (shadowingCases.empty())
    {
        return shaderSource;
    }

    std::string result = shaderSource;
    int offset = 0;

    for (const auto &shadow : shadowingCases)
    {
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

std::vector<ShaderPreprocessor::ShadowingInfo> ShaderPreprocessor::detectShadowing(const std::string &source)
{
    std::vector<ShadowingInfo> shadowingCases;
    std::vector<std::string> types = getShaderTypes();

    for (const auto &type : types)
    {
        std::string pattern = R"(\b)" + type + R"(\s+([a-zA-Z_]\w*)\s*=\s*([^;]+);)";
        std::regex declRegex(pattern);

        auto searchStart = source.cbegin();
        std::smatch match;

        while (std::regex_search(searchStart, source.cend(), match, declRegex))
        {
            std::string varName = match[1].str();
            std::string initExpression = match[2].str();

            std::string usagePattern = R"(\b)" + varName + R"(\s*[\.\[])";
            std::regex usageRegex(usagePattern);

            if (std::regex_search(initExpression, usageRegex))
            {
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

std::string ShaderPreprocessor::fixDivisionByZero(const std::string &shaderSource)
{
    std::vector<ForLoopInfo> dangerousLoops = detectDivisionByZeroInLoops(shaderSource);

    if (dangerousLoops.empty())
    {
        return shaderSource;
    }

    std::string result = shaderSource;
    int offset = 0;

    for (const auto &loop : dangerousLoops)
    {
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

std::vector<ShaderPreprocessor::ForLoopInfo> ShaderPreprocessor::detectDivisionByZeroInLoops(const std::string &source)
{
    std::vector<ForLoopInfo> dangerousLoops;

    std::regex forRegex(R"(\bfor\s*\(\s*(\w+)\s+([a-zA-Z_]\w*)\s*=\s*0\s*;[^;]+;[^)]+\))");

    auto searchStart = source.cbegin();
    std::smatch match;

    while (std::regex_search(searchStart, source.cend(), match, forRegex))
    {
        std::string loopVar = match[2].str();
        size_t forStart = static_cast<size_t>(match.position(0) + std::distance(source.cbegin(), searchStart));
        size_t forEnd = forStart + match.length(0);

        auto bodyOpt = findLoopBody(source, forEnd);
        if (!bodyOpt.has_value())
        {
            searchStart = match.suffix().first;
            continue;
        }

        auto [bodyStart, bodyEnd] = bodyOpt.value();
        std::string loopBody = source.substr(bodyStart, bodyEnd - bodyStart);

        std::string divPattern = R"(/\s*)" + loopVar + R"(\b)";
        std::regex divRegex(divPattern);

        if (std::regex_search(loopBody, divRegex))
        {
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

std::optional<std::pair<size_t, size_t>> ShaderPreprocessor::findLoopBody(const std::string &source, size_t forEnd) const
{
    size_t pos = forEnd;
    while (pos < source.length() && (std::isspace(source[pos]) || source[pos] == '\n'))
    {
        ++pos;
    }

    if (pos >= source.length())
    {
        return std::nullopt;
    }

    if (source[pos] == '{')
    {
        auto endOpt = findMatchingBrace(source, pos);
        if (!endOpt.has_value())
        {
            return std::nullopt;
        }
        return std::make_pair(pos, endOpt.value() + 1);
    }
    else
    {
        if (pos + 3 <= source.length() && source.substr(pos, 3) == "for")
        {
            auto nestedLoop = parseForLoopHeader(source, pos);
            if (nestedLoop.has_value())
            {
                return std::make_pair(pos, nestedLoop.value().bodyEnd);
            }
        }

        if (pos + 2 <= source.length() && source.substr(pos, 2) == "if")
        {
            size_t parenPos = source.find('(', pos);
            if (parenPos != std::string::npos)
            {
                auto parenEnd = findMatchingBrace(source, parenPos);
                if (parenEnd.has_value())
                {
                    auto ifBody = findLoopBody(source, parenEnd.value() + 1);
                    if (ifBody.has_value())
                    {
                        return std::make_pair(pos, ifBody.value().second);
                    }
                }
            }
        }

        size_t endPos = source.find(';', pos);
        if (endPos == std::string::npos)
        {
            return std::nullopt;
        }
        return std::make_pair(pos, endPos + 1);
    }
}

std::string ShaderPreprocessor::fixComplexForLoops(const std::string &shaderSource)
{
    std::vector<ComplexForLoopInfo> complexLoops = detectComplexForLoops(shaderSource);

    if (complexLoops.empty())
    {
        if (m_verbose)
        {
            std::cout << "No complex loops found." << std::endl;
        }
        return shaderSource;
    }

    std::string result = shaderSource;

    for (auto it = complexLoops.rbegin(); it != complexLoops.rend(); ++it)
    {
        const auto &loop = *it;

        int parenDepth = 0;
        bool hasMultipleInit = false;
        for (char c : loop.initialization)
        {
            if (c == '(')
                ++parenDepth;
            else if (c == ')')
                --parenDepth;
            else if (c == ',' && parenDepth == 0)
            {
                hasMultipleInit = true;
                break;
            }
        }

        parenDepth = 0;
        bool hasMultipleIncrement = false;
        for (char c : loop.increment)
        {
            if (c == '(')
                ++parenDepth;
            else if (c == ')')
                --parenDepth;
            else if (c == ',' && parenDepth == 0)
            {
                hasMultipleIncrement = true;
                break;
            }
        }

        if (!hasMultipleInit && !hasMultipleIncrement)
        {
            if (m_verbose)
            {
                std::cout << "Loop at position " << loop.forStart << " is not complex, skipping." << std::endl;
            }
            continue;
        }

        if (m_verbose)
        {
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

        if (hasMultipleInit)
        {
            bodyPrefix = loop.initialization + ";\n";
        }

        if (hasMultipleIncrement)
        {
            bodySuffix = loop.increment + ";\n";
        }

        if (loop.hasBlockBody)
        {
            std::string innerBody = loopBody.substr(1, loopBody.length() - 2);
            newBody = "{\n" + bodyPrefix + innerBody;
            if (!innerBody.empty() && innerBody.back() != '\n')
            {
                newBody += "\n";
            }
            newBody += bodySuffix + "}";
        }
        else
        {
            newBody = "{\n" + bodyPrefix + loopBody;
            if (!loopBody.empty() && loopBody.back() != '\n')
            {
                newBody += "\n";
            }
            newBody += bodySuffix + "}";
        }

        std::string replacement;
        if (hasMultipleInit)
        {
            replacement = "{\n" + loop.initialization + ";\n" + newForHeader + " " + newBody + "\n}";
        }
        else
        {
            replacement = newForHeader + " " + newBody;
        }

        result.replace(loop.forStart, loop.bodyEnd - loop.forStart, replacement);
    }

    return result;
}

std::vector<ShaderPreprocessor::ComplexForLoopInfo> ShaderPreprocessor::detectComplexForLoops(const std::string &source)
{
    std::vector<ComplexForLoopInfo> complexLoops;

    size_t pos = 0;
    int loopCount = 0;
    while (pos < source.length())
    {
        size_t forPos = source.find("for", pos);
        if (forPos == std::string::npos)
            break;

        bool isWordStart = (forPos == 0 || (!std::isalnum(source[forPos - 1]) && source[forPos - 1] != '_'));
        bool isWordEnd = (forPos + 3 >= source.length() || (!std::isalnum(source[forPos + 3]) && source[forPos + 3] != '_'));

        if (isWordStart && isWordEnd)
        {
            if (m_verbose)
            {
                std::cout << "  Found 'for' keyword at position " << forPos << std::endl;
            }

            auto loopOpt = parseForLoopHeader(source, forPos);
            if (loopOpt.has_value())
            {
                loopCount++;
                if (m_verbose)
                {
                    std::cout << "    Successfully parsed loop #" << loopCount << std::endl;
                }
                complexLoops.push_back(loopOpt.value());
                pos = loopOpt.value().headerEnd;
                continue;
            }
            else
            {
                if (m_verbose)
                {
                    std::cout << "    Failed to parse loop header" << std::endl;
                }
            }
        }

        pos = forPos + 3;
    }

    if (m_verbose)
    {
        std::cout << "  Total loops detected: " << complexLoops.size() << std::endl;
    }

    return complexLoops;
}

std::optional<ShaderPreprocessor::ComplexForLoopInfo> ShaderPreprocessor::parseForLoopHeader(
    const std::string &source, size_t forPos) const
{

    ComplexForLoopInfo info;
    info.forStart = forPos;

    size_t pos = forPos + 3;
    while (pos < source.length() && std::isspace(source[pos]))
        ++pos;

    if (pos >= source.length() || source[pos] != '(')
    {
        return std::nullopt;
    }

    size_t headerStart = pos + 1;

    int parenDepth = 1;
    ++pos;
    while (pos < source.length() && parenDepth > 0)
    {
        if (source[pos] == '(')
            ++parenDepth;
        else if (source[pos] == ')')
            --parenDepth;
        ++pos;
    }

    if (parenDepth != 0)
    {
        return std::nullopt;
    }

    size_t headerEnd = pos - 1;
    info.headerEnd = pos;

    std::string header = source.substr(headerStart, headerEnd - headerStart);

    std::vector<std::string> parts;
    std::string current;
    parenDepth = 0;

    for (char c : header)
    {
        if (c == '(')
            ++parenDepth;
        else if (c == ')')
            --parenDepth;
        else if (c == ';' && parenDepth == 0)
        {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current += c;
    }
    parts.push_back(current);

    if (parts.size() != 3)
    {
        return std::nullopt;
    }

    info.initialization = parts[0];
    info.condition = parts[1];
    info.increment = parts[2];

    auto trim = [](std::string &s)
    {
        size_t start = 0;
        while (start < s.length() && std::isspace(s[start]))
            ++start;
        size_t end = s.length();
        while (end > start && std::isspace(s[end - 1]))
            --end;
        s = s.substr(start, end - start);
    };

    trim(info.initialization);
    trim(info.condition);
    trim(info.increment);

    auto bodyOpt = findLoopBody(source, info.headerEnd);
    if (!bodyOpt.has_value())
    {
        return std::nullopt;
    }

    auto [bodyStart, bodyEnd] = bodyOpt.value();
    info.bodyStart = bodyStart;
    info.bodyEnd = bodyEnd;
    info.hasBlockBody = (source[bodyStart] == '{');
    info.forEnd = info.headerEnd;

    return info;
}

std::vector<std::string> ShaderPreprocessor::getShaderTypes() const
{
    if (m_language == ShaderLanguage::GLSL)
    {
        return {
            "float", "int", "uint", "bool",
            "vec2", "vec3", "vec4",
            "ivec2", "ivec3", "ivec4",
            "uvec2", "uvec3", "uvec4",
            "bvec2", "bvec3", "bvec4",
            "mat2", "mat3", "mat4",
            "mat2x2", "mat2x3", "mat2x4",
            "mat3x2", "mat3x3", "mat3x4",
            "mat4x2", "mat4x3", "mat4x4"};
    }
    else
    {
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
            "matrix", "vector"};
    }
}
