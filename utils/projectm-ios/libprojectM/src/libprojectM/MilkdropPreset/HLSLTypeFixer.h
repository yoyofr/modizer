#ifndef HLSL_TYPE_FIXER_H
#define HLSL_TYPE_FIXER_H

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <set>

/**
 * @brief Issue type detected in HLSL code
 */
enum class HLSLIssueType {
    INTEGER_DIVISION,           // int_var / literal
    INTEGER_IN_FLOAT_CONTEXT,   // integer literal in float expression
    INTEGER_VAR_IN_ARITHMETIC,  // int variable used without cast
    INTEGER_IN_VECTOR_CONSTRUCTOR, // float2(1, 2) instead of float2(1.0, 2.0)
    MIXED_TYPE_OPERATION,       // direct int/float mixing
    MISSING_FLOAT_SUFFIX        // 1.0 instead of 1.0f in some contexts
};

/**
 * @brief Represents a detected issue in HLSL code
 */
struct HLSLIssue {
    HLSLIssueType type;
    size_t position;        // Character position in source
    size_t line;            // Line number (1-indexed)
    size_t column;          // Column number (1-indexed)
    std::string text;       // The problematic text
    std::string suggestion; // Suggested fix
    std::string context;    // Surrounding code context
};

/**
 * @brief Options for HLSL type fixing
 */
struct HLSLFixOptions {
    bool autoFixIntegerDivision = true;
    bool autoFixIntegerLiterals = true;
    bool autoFixVectorConstructors = false;
    bool castIntVariables = true;
    bool addFloatSuffix = false;      // Use 1.0f instead of 1.0 (HLSL prefers 'f')
    bool preserveFormatting = true;
    bool fixMatrixConstructors = false; // Fix matrix constructors too
};

/**
 * @brief Main class for detecting and fixing HLSL type issues
 */
class HLSLTypeFixer {
public:
    HLSLTypeFixer();
    
    /**
     * @brief Detect all type issues in HLSL code
     * @param hlslCode The HLSL source code to analyze
     * @return Vector of detected issues
     */
    std::vector<HLSLIssue> detectIssues(const std::string& hlslCode);
    
    /**
     * @brief Automatically fix all detected issues
     * @param hlslCode The HLSL source code to fix
     * @param options Fixing options
     * @return Fixed HLSL code
     */
    std::string autoFix(const std::string& hlslCode, const HLSLFixOptions& options = HLSLFixOptions());
    
    /**
     * @brief Fix specific issue type only
     * @param hlslCode The HLSL source code to fix
     * @param issueType The specific issue type to fix
     * @return Fixed HLSL code
     */
    std::string fixIssueType(const std::string& hlslCode, HLSLIssueType issueType);
    
    /**
     * @brief Get human-readable description of issue type
     */
    static std::string getIssueTypeDescription(HLSLIssueType type);
    
    /**
     * @brief Set verbosity for debugging
     */
    void setVerbose(bool verbose) { m_verbose = verbose; }

private:
    // Internal methods
    void findIntegerVariables(const std::string& hlslCode);
    void detectIntegerDivision(const std::string& hlslCode, std::vector<HLSLIssue>& issues);
    void detectIntegerInFloatContext(const std::string& hlslCode, std::vector<HLSLIssue>& issues);
    void detectIntegerInVectorConstructor(const std::string& hlslCode, std::vector<HLSLIssue>& issues);
    void detectMixedTypeOperations(const std::string& hlslCode, std::vector<HLSLIssue>& issues);
    
    std::string fixIntegerDivision(const std::string& hlslCode, const HLSLFixOptions& options);
    std::string fixIntegerLiterals(const std::string& hlslCode, const HLSLFixOptions& options);
    std::string fixVectorConstructors(const std::string& hlslCode, const HLSLFixOptions& options);
    std::string fixIntegerVariables(const std::string& hlslCode, const HLSLFixOptions& options);
    
    void calculateLineColumn(const std::string& code, size_t position, size_t& line, size_t& column);
    std::string getContext(const std::string& code, size_t position, size_t contextLength = 40);
    bool isInComment(const std::string& code, size_t position);
    bool isInString(const std::string& code, size_t position);
    
    // Member variables
    std::set<std::string> m_intVariables;
    std::set<std::string> m_floatVariables;
    bool m_verbose;
    
    // HLSL type keywords
    static const std::vector<std::string> HLSL_INT_TYPES;
    static const std::vector<std::string> HLSL_FLOAT_TYPES;
    static const std::vector<std::string> HLSL_VECTOR_TYPES;
    static const std::vector<std::string> HLSL_MATRIX_TYPES;
};

#endif // HLSL_TYPE_FIXER_H
