//
//  ShaderPreprocessor.h
//  projectm-ios
//
//  Created by Yohann Magnien David on 30/10/2025.
//

#ifndef SHADER_PREPROCESSOR_H
#define SHADER_PREPROCESSOR_H

#include <string>
#include <vector>
#include <optional>

enum class ShaderLanguage {
    GLSL,
    HLSL
};

class ShaderPreprocessor {
public:
    /**
     * Constructor
     * @param language The shader language to process (GLSL or HLSL)
     */
    explicit ShaderPreprocessor(ShaderLanguage language = ShaderLanguage::GLSL);

    /**
     * Remove functions that should return a value but don't have a return statement
     * @param shaderSource The shader source code
     * @return The processed shader source with invalid functions removed
     */
    [[nodiscard]] std::string removeInvalidFunctions(const std::string& shaderSource);

    /**
     * Fix variable shadowing where a variable is declared using a member of itself
     * Example: float aspect = aspect.x/aspect.y; -> float aspect_float = aspect.x/aspect.y;
     * @param shaderSource The shader source code
     * @return The processed shader source with shadowing fixed
     */
    [[nodiscard]] std::string fixVariableShadowing(const std::string& shaderSource);

    /**
     * Fix division by zero risks in for loops
     * Example: for(float i = 0; ...) { ... / i ...} -> for(float i = 1; ...) { ... / i ...}
     * @param shaderSource The shader source code
     * @return The processed shader source with division by zero fixed
     */
    [[nodiscard]] std::string fixDivisionByZero(const std::string& shaderSource);

    /**
     * Fix complex for loops with multiple statements in increment section
     * Moves complex increment logic into the loop body
     * @param shaderSource The shader source code
     * @return The processed shader source with simplified for loops
     */
    [[nodiscard]] std::string fixComplexForLoops(const std::string& shaderSource);

    /**
     * Rename keywords when used as variable names
     * Example: "float sample = ..." becomes "float sample_var = ..."
     * @param shaderSource The shader source code
     * @param keywords List of keywords to rename when used as variables
     * @param suffix Suffix to append to renamed variables (default: "_var")
     * @return The processed shader source with renamed variables
     */
    [[nodiscard]] std::string renameKeywordsAsVariables(const std::string& shaderSource, 
                                                         const std::vector<std::string>& keywords,
                                                         const std::string& suffix = "_var");

    /**
     * Apply all preprocessing steps
     * @param shaderSource The shader source code
     * @return The fully processed shader source
     */
    [[nodiscard]] std::string preprocess(const std::string& shaderSource);

    /**
     * Set the shader language
     */
    void setLanguage(ShaderLanguage language) noexcept;

    /**
     * Enable or disable verbose debug output
     */
    void setVerbose(bool verbose) noexcept;

private:
    struct FunctionInfo {
        size_t startPos;
        size_t length;
        std::string returnType;
        bool shouldReturnValue;
        bool hasReturn;
        std::string body;
    };

    /**
     * Extract all functions from the shader source
     */
    [[nodiscard]] std::vector<FunctionInfo> extractFunctions(const std::string& source);

    /**
     * Find the closing brace that matches the opening brace at openBracePos
     */
    [[nodiscard]] std::optional<size_t> findMatchingBrace(const std::string& source, size_t openBracePos) const;

    /**
     * Check if a function body contains a return statement with a value
     */
    [[nodiscard]] bool checkForReturn(const std::string& body) const;

    /**
     * Remove comments and strings from source to avoid false positives
     */
    [[nodiscard]] std::string removeCommentsAndStrings(const std::string& source) const;

    struct ShadowingInfo {
        std::string varType;
        std::string varName;
        std::string originalName;
        size_t declarationStart;
        size_t declarationEnd;
    };

    /**
     * Detect variable declarations that shadow themselves (e.g., float aspect = aspect.x)
     */
    [[nodiscard]] std::vector<ShadowingInfo> detectShadowing(const std::string& source);

    /**
     * Get list of shader types for the current language
     */
    [[nodiscard]] std::vector<std::string> getShaderTypes() const;

    struct ForLoopInfo {
        std::string loopVariable;
        size_t forStatementStart;
        size_t forStatementEnd;
        size_t loopBodyStart;
        size_t loopBodyEnd;
        std::string initValue;
    };

    /**
     * Detect for loops that start at 0 and divide by the loop variable
     */
    [[nodiscard]] std::vector<ForLoopInfo> detectDivisionByZeroInLoops(const std::string& source);

    /**
     * Find the loop body for a given for statement
     */
    [[nodiscard]] std::optional<std::pair<size_t, size_t>> findLoopBody(const std::string& source, size_t forEnd) const;

    struct ComplexForLoopInfo {
        size_t forStart;
        size_t forEnd;
        size_t headerEnd;  // End of for(...) before body
        std::string initialization;
        std::string condition;
        std::string increment;
        size_t bodyStart;
        size_t bodyEnd;
        bool hasBlockBody;  // true if body is {...}, false if single statement
    };

    /**
     * Detect for loops with complex increment sections (multiple statements separated by commas)
     */
    [[nodiscard]] std::vector<ComplexForLoopInfo> detectComplexForLoops(const std::string& source);

    /**
     * Parse a for loop header and extract its components
     */
    [[nodiscard]] std::optional<ComplexForLoopInfo> parseForLoopHeader(const std::string& source, size_t forPos) const;

    struct KeywordUsageInfo {
        std::string keyword;
        std::string newName;
        size_t declarationStart;
        size_t declarationEnd;
        std::vector<std::pair<size_t, size_t>> usages;  // Positions where keyword is used as variable
    };

    /**
     * Detect keywords used as variable names
     */
    [[nodiscard]] std::vector<KeywordUsageInfo> detectKeywordUsages(
        const std::string& source,
        const std::vector<std::string>& keywords,
        const std::string& suffix);

    ShaderLanguage m_language;
    bool m_verbose = false;
    mutable std::vector<std::string> m_cachedTypes;
    mutable ShaderLanguage m_cachedTypesLanguage = ShaderLanguage::GLSL;
};

#endif // SHADER_PREPROCESSOR_H
