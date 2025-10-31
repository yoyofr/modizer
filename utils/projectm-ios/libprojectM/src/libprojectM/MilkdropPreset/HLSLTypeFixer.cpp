#include "HLSLTypeFixer.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// Static member initialization
const std::vector<std::string> HLSLTypeFixer::HLSL_INT_TYPES = {
    "int", "uint", "int1", "int2", "int3", "int4",
    "uint1", "uint2", "uint3", "uint4",
    "dword"  // HLSL specific
};

const std::vector<std::string> HLSLTypeFixer::HLSL_FLOAT_TYPES = {
    "float", "half", "double",
    "float1", "float2", "float3", "float4",
    "half1", "half2", "half3", "half4",
    "double1", "double2", "double3", "double4"
};

const std::vector<std::string> HLSLTypeFixer::HLSL_VECTOR_TYPES = {
    "float1", "float2", "float3", "float4",
    "half1", "half2", "half3", "half4",
    "double1", "double2", "double3", "double4",
    "int1", "int2", "int3", "int4",
    "uint1", "uint2", "uint3", "uint4"
};

const std::vector<std::string> HLSLTypeFixer::HLSL_MATRIX_TYPES = {
    "float2x2", "float2x3", "float2x4",
    "float3x2", "float3x3", "float3x4",
    "float4x2", "float4x3", "float4x4",
    "matrix"  // Generic matrix type
};

HLSLTypeFixer::HLSLTypeFixer() : m_verbose(false) {}

std::string HLSLTypeFixer::getIssueTypeDescription(HLSLIssueType type) {
    switch (type) {
        case HLSLIssueType::INTEGER_DIVISION:
            return "Integer division in float context";
        case HLSLIssueType::INTEGER_IN_FLOAT_CONTEXT:
            return "Integer literal in float expression";
        case HLSLIssueType::INTEGER_VAR_IN_ARITHMETIC:
            return "Integer variable in float arithmetic";
        case HLSLIssueType::INTEGER_IN_VECTOR_CONSTRUCTOR:
            return "Integer literal in vector constructor";
        case HLSLIssueType::MIXED_TYPE_OPERATION:
            return "Mixed int/float operation";
        case HLSLIssueType::MISSING_FLOAT_SUFFIX:
            return "Missing 'f' suffix on float literal";
        default:
            return "Unknown issue";
    }
}

void HLSLTypeFixer::calculateLineColumn(const std::string& code, size_t position, size_t& line, size_t& column) {
    line = 1;
    column = 1;
    for (size_t i = 0; i < position && i < code.length(); ++i) {
        if (code[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
}

std::string HLSLTypeFixer::getContext(const std::string& code, size_t position, size_t contextLength) {
    if (position >= code.length()) {
        return "";
    }
    size_t start = (position > contextLength) ? position - contextLength : 0;
    size_t end = std::min(position + contextLength, code.length());
    if (start >= code.length()) start = 0;
    if (end > code.length()) end = code.length();
    if (start >= end) return "";
    return code.substr(start, end - start);
}

bool HLSLTypeFixer::isInComment(const std::string& code, size_t position) {
    // Check for single-line comment
    size_t lineStart = code.rfind('\n', position);
    if (lineStart == std::string::npos) lineStart = 0;
    size_t commentPos = code.find("//", lineStart);
    if (commentPos != std::string::npos && commentPos < position) {
        size_t lineEnd = code.find('\n', position);
        if (lineEnd == std::string::npos || position < lineEnd) {
            return true;
        }
    }
    
    // Check for multi-line comment
    size_t blockStart = code.rfind("/*", position);
    size_t blockEnd = code.rfind("*/", position);
    if (blockStart != std::string::npos) {
        if (blockEnd == std::string::npos || blockStart > blockEnd) {
            return true;
        }
    }
    
    return false;
}

bool HLSLTypeFixer::isInString(const std::string& code, size_t position) {
    int quoteCount = 0;
    for (size_t i = 0; i < position && i < code.length(); ++i) {
        if (code[i] == '"' && (i == 0 || code[i-1] != '\\')) {
            quoteCount++;
        }
    }
    return (quoteCount % 2) == 1;
}

void HLSLTypeFixer::findIntegerVariables(const std::string& hlslCode) {
    m_intVariables.clear();
    m_floatVariables.clear();
    
    // Find integer variable declarations
    for (const auto& intType : HLSL_INT_TYPES) {
        std::regex intVarRegex("\\b" + intType + "\\s+(\\w+)\\s*[=;,:]");
        std::smatch match;
        std::string::const_iterator searchStart(hlslCode.cbegin());
        
        while (std::regex_search(searchStart, hlslCode.cend(), match, intVarRegex)) {
            size_t pos = match.position() + (searchStart - hlslCode.cbegin());
            if (!isInComment(hlslCode, pos) && !isInString(hlslCode, pos)) {
                m_intVariables.insert(match[1].str());
            }
            searchStart = match.suffix().first;
        }
    }
    
    // Find float variable declarations
    for (const auto& floatType : HLSL_FLOAT_TYPES) {
        std::regex floatVarRegex("\\b" + floatType + "\\s+(\\w+)\\s*[=;,:]");
        std::smatch match;
        std::string::const_iterator searchStart(hlslCode.cbegin());
        
        while (std::regex_search(searchStart, hlslCode.cend(), match, floatVarRegex)) {
            size_t pos = match.position() + (searchStart - hlslCode.cbegin());
            if (!isInComment(hlslCode, pos) && !isInString(hlslCode, pos)) {
                m_floatVariables.insert(match[1].str());
            }
            searchStart = match.suffix().first;
        }
    }
}

void HLSLTypeFixer::detectIntegerDivision(const std::string& hlslCode, std::vector<HLSLIssue>& issues) {
    // Detect int_var / integer_literal
    for (const auto& intVar : m_intVariables) {
        std::regex divRegex("\\b(" + intVar + ")\\s*/\\s*(\\d+)(?![\\d\\.fF])");
        std::smatch match;
        std::string::const_iterator searchStart(hlslCode.cbegin());
        
        while (std::regex_search(searchStart, hlslCode.cend(), match, divRegex)) {
            size_t pos = match.position() + (searchStart - hlslCode.cbegin());
            
            if (!isInComment(hlslCode, pos) && !isInString(hlslCode, pos)) {
                HLSLIssue issue;
                issue.type = HLSLIssueType::INTEGER_DIVISION;
                issue.position = pos;
                calculateLineColumn(hlslCode, pos, issue.line, issue.column);
                issue.text = match[0].str();
                issue.suggestion = "(float)" + match[1].str() + " / " + match[2].str() + ".0f";
                issue.context = getContext(hlslCode, pos);
                issues.push_back(issue);
            }
            
            searchStart = match.suffix().first;
        }
    }
}

void HLSLTypeFixer::detectIntegerInFloatContext(const std::string& hlslCode, std::vector<HLSLIssue>& issues) {
    // Detect integer literals in arithmetic operations with floats
    // Simple pattern without lookbehind
    std::regex intLitRegex("\\b(\\d+)(?![\\d\\.fFeE])");
    
    std::smatch match;
    std::string::const_iterator searchStart(hlslCode.cbegin());
    
    while (std::regex_search(searchStart, hlslCode.cend(), match, intLitRegex)) {
        size_t pos = match.position() + (searchStart - hlslCode.cbegin());
        
        if (!isInComment(hlslCode, pos) && !isInString(hlslCode, pos)) {
            // Check if preceded by a dot (part of decimal number)
            if (pos > 0 && hlslCode[pos - 1] == '.') {
                searchStart = match.suffix().first;
                continue;
            }
            
            // Check if part of scientific notation (e.g., 1e-3, 1e+5, 1E10)
            if (pos > 1) {
                char prev1 = hlslCode[pos - 1];
                char prev2 = hlslCode[pos - 2];
                // Check for patterns like "e-3", "e+3", "E-3", "E+3"
                if ((prev1 == '-' || prev1 == '+') && (prev2 == 'e' || prev2 == 'E')) {
                    searchStart = match.suffix().first;
                    continue;
                }
                // Check for patterns like "e3", "E3"
                if (prev1 == 'e' || prev1 == 'E') {
                    // Make sure it's preceded by a digit (part of number)
                    if (pos > 2 && isdigit(hlslCode[pos - 2])) {
                        searchStart = match.suffix().first;
                        continue;
                    }
                }
            }
            
            // Check if this is in a float context
            size_t lineStart = hlslCode.rfind('\n', pos);
            if (lineStart == std::string::npos) lineStart = 0;
            size_t lineEnd = hlslCode.find('\n', pos);
            if (lineEnd == std::string::npos) lineEnd = hlslCode.length();
            
            std::string line = hlslCode.substr(lineStart, lineEnd - lineStart);
            
            // Check if line contains float indicators
            bool hasFloatContext = false;
            for (const auto& vecType : HLSL_VECTOR_TYPES) {
                if (line.find(vecType) != std::string::npos) {
                    hasFloatContext = true;
                    break;
                }
            }
            
            // Also check for decimal points or 'f' suffix in the line
            size_t dotPos = line.find('.');
            if (dotPos != std::string::npos && dotPos + 1 < line.length()) {
                char after = line[dotPos + 1];
                if (isdigit(after)) {
                    hasFloatContext = true;
                }
            }
            if (line.find('f') != std::string::npos || line.find('F') != std::string::npos) {
                hasFloatContext = true;
            }
            
            if (hasFloatContext) {
                // Check if preceded or followed by arithmetic operators
                bool beforeOp = false;
                bool afterOp = false;
                
                if (pos > 0) {
                    size_t checkPos = pos - 1;
                    while (checkPos > 0 && isspace(hlslCode[checkPos])) checkPos--;
                    if (hlslCode[checkPos] == '(' || hlslCode[checkPos] == ',' ||
                        hlslCode[checkPos] == '+' || hlslCode[checkPos] == '-' ||
                        hlslCode[checkPos] == '*' || hlslCode[checkPos] == '/') {
                        beforeOp = true;
                    }
                }
                
                if (pos + match.length() < hlslCode.length()) {
                    size_t checkPos = pos + match.length();
                    while (checkPos < hlslCode.length() && isspace(hlslCode[checkPos])) checkPos++;
                    if (checkPos < hlslCode.length() && (
                        hlslCode[checkPos] == ')' || hlslCode[checkPos] == ',' ||
                        hlslCode[checkPos] == '+' || hlslCode[checkPos] == '-' ||
                        hlslCode[checkPos] == '*' || hlslCode[checkPos] == '/')) {
                        afterOp = true;
                    }
                }
                
                if (beforeOp || afterOp) {
                    HLSLIssue issue;
                    issue.type = HLSLIssueType::INTEGER_IN_FLOAT_CONTEXT;
                    issue.position = pos;
                    calculateLineColumn(hlslCode, pos, issue.line, issue.column);
                    issue.text = match[1].str();
                    issue.suggestion = match[1].str() + ".0f";
                    issue.context = getContext(hlslCode, pos);
                    issues.push_back(issue);
                }
            }
        }
        
        searchStart = match.suffix().first;
    }
}

void HLSLTypeFixer::detectIntegerInVectorConstructor(const std::string& hlslCode, std::vector<HLSLIssue>& issues) {
    // Detect integers in vector constructors like float2(1, 2)
    for (const auto& vecType : HLSL_VECTOR_TYPES) {
        // Skip integer vector types
        if (vecType.find("int") != std::string::npos || vecType.find("uint") != std::string::npos) {
            continue;
        }
        
        std::regex vecRegex("\\b" + vecType + "\\s*\\(([^)]*)\\)");
        std::smatch match;
        std::string::const_iterator searchStart(hlslCode.cbegin());
        
        while (std::regex_search(searchStart, hlslCode.cend(), match, vecRegex)) {
            size_t pos = match.position() + (searchStart - hlslCode.cbegin());
            
            if (!isInComment(hlslCode, pos) && !isInString(hlslCode, pos)) {
                std::string args = match[1].str();
                
                // Find integer literals in arguments (excluding scientific notation)
                std::regex intLitRegex("\\b(\\d+)(?![\\d\\.fFeE])");
                std::smatch intMatch;
                std::string::const_iterator argStart(args.cbegin());
                
                while (std::regex_search(argStart, args.cend(), intMatch, intLitRegex)) {
                    // Check if this integer is preceded by a dot
                    size_t relativePos = intMatch.position() + (argStart - args.cbegin());
                    if (relativePos > 0 && args[relativePos - 1] == '.') {
                        argStart = intMatch.suffix().first;
                        continue;
                    }
                    
                    // Check if part of scientific notation
                    if (relativePos > 1) {
                        char prev1 = args[relativePos - 1];
                        char prev2 = args[relativePos - 2];
                        if ((prev1 == '-' || prev1 == '+') && (prev2 == 'e' || prev2 == 'E')) {
                            argStart = intMatch.suffix().first;
                            continue;
                        }
                        if ((prev1 == 'e' || prev1 == 'E') && relativePos > 2 && isdigit(args[relativePos - 2])) {
                            argStart = intMatch.suffix().first;
                            continue;
                        }
                    }
                    
                    size_t intPos = pos + match.position(1) + relativePos;
                    
                    HLSLIssue issue;
                    issue.type = HLSLIssueType::INTEGER_IN_VECTOR_CONSTRUCTOR;
                    issue.position = intPos;
                    calculateLineColumn(hlslCode, intPos, issue.line, issue.column);
                    issue.text = intMatch[1].str();
                    issue.suggestion = intMatch[1].str() + ".0f";
                    issue.context = getContext(hlslCode, intPos);
                    issues.push_back(issue);
                    
                    argStart = intMatch.suffix().first;
                }
            }
            
            searchStart = match.suffix().first;
        }
    }
}

void HLSLTypeFixer::detectMixedTypeOperations(const std::string& hlslCode, std::vector<HLSLIssue>& issues) {
    // Detect int variables used directly in float arithmetic
    for (const auto& intVar : m_intVariables) {
        // Look for int_var in arithmetic with float literals or variables
        std::regex mixedRegex("(\\d+\\.\\d*[fF]?)[\\s]*([+\\-*/])[\\s]*\\b(" + intVar + ")\\b(?!\\s*\\()");
        
        std::smatch match;
        std::string::const_iterator searchStart(hlslCode.cbegin());
        
        while (std::regex_search(searchStart, hlslCode.cend(), match, mixedRegex)) {
            size_t pos = match.position() + (searchStart - hlslCode.cbegin());
            
            if (!isInComment(hlslCode, pos) && !isInString(hlslCode, pos)) {
                HLSLIssue issue;
                issue.type = HLSLIssueType::INTEGER_VAR_IN_ARITHMETIC;
                issue.position = pos;
                calculateLineColumn(hlslCode, pos, issue.line, issue.column);
                issue.text = match[0].str();
                issue.suggestion = "Use (float)" + intVar + " in the expression";
                issue.context = getContext(hlslCode, pos);
                issues.push_back(issue);
            }
            
            searchStart = match.suffix().first;
        }
    }
}

std::vector<HLSLIssue> HLSLTypeFixer::detectIssues(const std::string& hlslCode) {
    std::vector<HLSLIssue> issues;
    
    // First, find all integer and float variables
    findIntegerVariables(hlslCode);
    
    if (m_verbose) {
        std::cout << "Found " << m_intVariables.size() << " integer variables and "
                  << m_floatVariables.size() << " float variables" << std::endl;
    }
    
    // Detect various issues
    detectIntegerDivision(hlslCode, issues);
    detectIntegerInFloatContext(hlslCode, issues);
    detectIntegerInVectorConstructor(hlslCode, issues);
    detectMixedTypeOperations(hlslCode, issues);
    
    // Sort issues by position
    std::sort(issues.begin(), issues.end(),
        [](const HLSLIssue& a, const HLSLIssue& b) { return a.position < b.position; });
    
    return issues;
}

std::string HLSLTypeFixer::fixIntegerDivision(const std::string& hlslCode, const HLSLFixOptions& options) {
    std::string result = hlslCode;
    
    // Fix int_var / integer_literal
    for (const auto& intVar : m_intVariables) {
        std::regex divRegex("\\b(" + intVar + ")\\s*/\\s*(\\d+)(?![\\d\\.fF])");
        
        std::string replacement = options.addFloatSuffix ?
            "(float)$1 / $2.0f" : "(float)$1 / $2.0";
        
        result = std::regex_replace(result, divRegex, replacement);
    }
    
    return result;
}

std::string HLSLTypeFixer::fixIntegerLiterals(const std::string& hlslCode, const HLSLFixOptions& options) {
    std::string result = hlslCode;
    
    // Fix integer literals in arithmetic operations (excluding scientific notation)
    std::regex intAfterOpRegex("([+\\-*/,\\(])\\s*(\\d+)(?![\\d\\.fFeE])");
    
    std::string suffix = options.addFloatSuffix ? ".0f" : ".0";
    
    // Manual replacement to check for preceding contexts
    std::string output;
    std::smatch match;
    auto searchStart = result.cbegin();
    size_t lastPos = 0;
    
    while (std::regex_search(searchStart, result.cend(), match, intAfterOpRegex)) {
        size_t matchPos = match.position() + (searchStart - result.cbegin());
        size_t intPos = matchPos + match[1].length();
        
        // Skip if this is part of scientific notation (e.g., 1e-3)
        bool isScientific = false;
        if (intPos > 2) {
            char op = match[1].str()[0];
            if (op == '-' || op == '+') {
                // Check if preceded by 'e' or 'E'
                size_t checkPos = matchPos - 1;
                while (checkPos > 0 && isspace(result[checkPos])) checkPos--;
                if (checkPos > 0 && (result[checkPos] == 'e' || result[checkPos] == 'E')) {
                    // Make sure the 'e' is part of a number
                    size_t beforeE = checkPos - 1;
                    while (beforeE > 0 && isspace(result[beforeE])) beforeE--;
                    if (beforeE > 0 && (isdigit(result[beforeE]) || result[beforeE] == '.')) {
                        isScientific = true;
                    }
                }
            }
        }
        
        if (isScientific) {
            output += result.substr(lastPos, matchPos + match.length() - lastPos);
            lastPos = matchPos + match.length();
            searchStart = match.suffix().first;
            continue;
        }
        
        output += result.substr(lastPos, matchPos - lastPos);
        output += match[1].str() + " " + match[2].str() + suffix;
        lastPos = matchPos + match.length();
        
        searchStart = match.suffix().first;
    }
    output += result.substr(lastPos);
    
    return output;
}

std::string HLSLTypeFixer::fixVectorConstructors(const std::string& hlslCode, const HLSLFixOptions& options) {
    std::string result = hlslCode;
    
    // Fix integers in vector constructors
    for (const auto& vecType : HLSL_VECTOR_TYPES) {
        // Skip integer vector types
        if (vecType.find("int") != std::string::npos || vecType.find("uint") != std::string::npos) {
            continue;
        }
        
        // Match vector constructor and its contents
        std::regex vecRegex("\\b(" + vecType + "\\s*\\()([^)]*)\\)");
        
        std::string replacement;
        std::smatch match;
        std::string temp = result;
        result.clear();
        
        auto searchStart = temp.cbegin();
        size_t lastPos = 0;
        
        while (std::regex_search(searchStart, temp.cend(), match, vecRegex)) {
            size_t matchPos = match.position() + (searchStart - temp.cbegin());
            result += temp.substr(lastPos, matchPos - lastPos);
            result += match[1].str();
            
            // Fix integers in the arguments (excluding scientific notation)
            std::string args = match[2].str();
            std::regex intRegex("\\b(\\d+)(?![\\d\\.fFeE])");
            
            // Manual replacement to check for preceding dots and scientific notation
            std::string fixedArgs;
            std::smatch intMatch;
            auto argStart = args.cbegin();
            size_t argLastPos = 0;
            
            while (std::regex_search(argStart, args.cend(), intMatch, intRegex)) {
                size_t intMatchPos = intMatch.position() + (argStart - args.cbegin());
                
                // Check if preceded by dot
                if (intMatchPos > 0 && args[intMatchPos - 1] == '.') {
                    fixedArgs += args.substr(argLastPos, intMatchPos + intMatch.length() - argLastPos);
                    argLastPos = intMatchPos + intMatch.length();
                    argStart = intMatch.suffix().first;
                    continue;
                }
                
                // Check if part of scientific notation
                bool isScientific = false;
                if (intMatchPos > 1) {
                    char prev1 = args[intMatchPos - 1];
                    char prev2 = args[intMatchPos - 2];
                    if ((prev1 == '-' || prev1 == '+') && (prev2 == 'e' || prev2 == 'E')) {
                        isScientific = true;
                    } else if ((prev1 == 'e' || prev1 == 'E') && intMatchPos > 2 && isdigit(args[intMatchPos - 2])) {
                        isScientific = true;
                    }
                }
                
                if (isScientific) {
                    fixedArgs += args.substr(argLastPos, intMatchPos + intMatch.length() - argLastPos);
                    argLastPos = intMatchPos + intMatch.length();
                    argStart = intMatch.suffix().first;
                    continue;
                }
                
                fixedArgs += args.substr(argLastPos, intMatchPos - argLastPos);
                std::string suffix = options.addFloatSuffix ? ".0f" : ".0";
                fixedArgs += intMatch[1].str() + suffix;
                argLastPos = intMatchPos + intMatch.length();
                argStart = intMatch.suffix().first;
            }
            fixedArgs += args.substr(argLastPos);
            
            result += fixedArgs + ")";
            
            lastPos = matchPos + match.length();
            searchStart = match.suffix().first;
        }
        
        result += temp.substr(lastPos);
        temp = result;
    }
    
    return result;
}

std::string HLSLTypeFixer::fixIntegerVariables(const std::string& hlslCode, const HLSLFixOptions& options) {
    std::string result = hlslCode;
    
    // Cast integer variables when used in float contexts
    for (const auto& intVar : m_intVariables) {
        // Look for patterns like: float_literal op int_var
        std::regex floatContext("(\\d+\\.\\d*[fF]?)\\s*([+\\-*/])\\s*\\b(" + intVar + ")\\b(?!\\s*\\()");
        
        std::string temp;
        std::smatch match;
        auto searchStart = result.cbegin();
        size_t lastPos = 0;
        
        while (std::regex_search(searchStart, result.cend(), match, floatContext)) {
            size_t matchPos = match.position() + (searchStart - result.cbegin());
            
            // Check if not already casted
            std::string before = result.substr(std::max(0, (int)matchPos - 7), 7);
            if (before.find("float)") != std::string::npos) {
                // Already casted, skip
                searchStart = match.suffix().first;
                continue;
            }
            
            temp += result.substr(lastPos, matchPos - lastPos);
            temp += match[1].str() + " " + match[2].str() + " (float)" + match[3].str();
            lastPos = matchPos + match.length();
            searchStart = match.suffix().first;
        }
        temp += result.substr(lastPos);
        result = temp;
    }
    
    return result;
}

std::string HLSLTypeFixer::autoFix(const std::string& hlslCode, const HLSLFixOptions& options) {
    std::string result = hlslCode;
    
    // Find variables first
    findIntegerVariables(result);
    
    if (options.autoFixIntegerDivision) {
        result = fixIntegerDivision(result, options);
    }
    
    if (options.autoFixVectorConstructors) {
        result = fixVectorConstructors(result, options);
    }
    
    if (options.castIntVariables) {
        result = fixIntegerVariables(result, options);
    }
    
    if (options.autoFixIntegerLiterals) {
        result = fixIntegerLiterals(result, options);
    }
    
    return result;
}

std::string HLSLTypeFixer::fixIssueType(const std::string& hlslCode, HLSLIssueType issueType) {
    HLSLFixOptions options;
    options.autoFixIntegerDivision = (issueType == HLSLIssueType::INTEGER_DIVISION);
    options.autoFixIntegerLiterals = (issueType == HLSLIssueType::INTEGER_IN_FLOAT_CONTEXT);
    options.autoFixVectorConstructors = (issueType == HLSLIssueType::INTEGER_IN_VECTOR_CONSTRUCTOR);
    options.castIntVariables = (issueType == HLSLIssueType::INTEGER_VAR_IN_ARITHMETIC);
    
    return autoFix(hlslCode, options);
}
