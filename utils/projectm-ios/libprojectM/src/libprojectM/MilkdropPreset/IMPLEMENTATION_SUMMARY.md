# Implementation Summary: Keyword Renaming Feature

## What Was Added

I've added a new feature to the `ShaderPreprocessor` class that automatically renames reserved keywords when they're used as variable names. This is particularly useful for HLSL shaders where keywords like `sample` might be used as variable names.

## Files Modified

### 1. ShaderPreprocessor.h
- Added public method: `renameKeywordsAsVariables()`
- Added private struct: `KeywordUsageInfo`
- Added private method: `detectKeywordUsages()`

### 2. ShaderPreprocessor.cpp
- Implemented `renameKeywordsAsVariables()` method
- Implemented `detectKeywordUsages()` helper method
- Integrated keyword renaming into `preprocess()` pipeline as Step 5

## Key Features

### Generic and Flexible
- Takes a **vector of keywords** rather than being hardcoded
- Supports **custom suffix** (defaults to "_var")
- Can be called **directly** or **automatically** via preprocess()

### Smart Detection
- Only renames when keywords are used as **variable declarations**
- Requires pattern: `<type> <keyword> = ...;`
- Won't rename function names, parameters, or other non-variable usages

### Safe Renaming
- Uses **whole-word matching** to avoid partial replacements
- Processes in **reverse order** to maintain correct string indices
- Renames **all usages** after the declaration

## How to Use

### Option 1: Automatic (via preprocess)
```cpp
ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
std::string result = preprocessor.preprocess(shaderCode);
// Automatically renames "sample" keyword
```

### Option 2: Direct Call with Custom Keywords
```cpp
ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
std::vector<std::string> keywords = {"sample", "input", "output"};
std::string result = preprocessor.renameKeywordsAsVariables(shaderCode, keywords);
```

### Option 3: Custom Suffix
```cpp
std::vector<std::string> keywords = {"sample"};
std::string result = preprocessor.renameKeywordsAsVariables(
    shaderCode, 
    keywords, 
    "_renamed"  // Custom suffix
);
```

## Example Transformation

### Input
```hlsl
float3 sample = tex2D(sampler_main, uv);
ret = sample*sample*sample;
```

### Output
```hlsl
float3 sample_var = tex2D(sampler_main, uv);
ret = sample_var*sample_var*sample_var;
```

## Integration with Preprocessing Pipeline

The feature is integrated as **Step 5** in the `preprocess()` method:

1. Remove invalid functions
2. Fix variable shadowing
3. Fix division by zero
4. Clean preprocessor directives
5. **Rename keywords used as variables** ← NEW
6. Fix complex for loops

The keyword renaming is **only applied for HLSL** (`m_language == ShaderLanguage::HLSL`).

## Implementation Details

### Data Structures

```cpp
struct KeywordUsageInfo {
    std::string keyword;           // Original keyword
    std::string newName;           // New name with suffix
    size_t declarationStart;       // Start of declaration
    size_t declarationEnd;         // End of declaration (including semicolon)
    std::vector<std::pair<size_t, size_t>> usages;  // Future use
};
```

### Algorithm

1. **Detection Phase** (`detectKeywordUsages`):
   - For each keyword in the list
   - For each shader type (float, vec3, etc.)
   - Search for pattern: `<type> <keyword> = ...;`
   - Store declaration positions

2. **Renaming Phase** (`renameKeywordsAsVariables`):
   - Process usages in **reverse order** (to maintain indices)
   - For each usage:
     - Rename in the declaration
     - Find and rename all subsequent occurrences
     - Use whole-word matching
     - Track offset for correct positioning

### Performance Considerations

- **Pre-allocated vectors** for efficiency
- **Manual string parsing** instead of regex for better performance
- **Whole-word checks** using character type validation
- **Reverse iteration** to avoid index recalculation
- **Reserved capacity** to minimize reallocations

## Testing

A test file (`test_keyword_rename.cpp`) demonstrates:
1. Basic sample variable usage
2. Multiple usages of the same variable
3. Custom keywords (input, output)
4. Function names should NOT be renamed

## Consistency with Existing Code

The implementation follows the same patterns as other methods in the class:
- Uses `[[nodiscard]]` attribute
- Manual parsing for performance
- Whole-word matching logic
- Reverse iteration for string modifications
- Pre-allocated vectors
- Clear struct definitions
- Detailed comments

## Future Enhancements

Potential improvements that could be added:
1. Support for multiple declarations of the same keyword
2. Scope-aware renaming (different scopes can have same variable name)
3. Detection of function parameters that use keywords
4. Statistics on how many keywords were renamed
5. Option to preserve original names in comments

## Why This Design?

### Generic Keyword List
- Different projects may have different problematic keywords
- Allows flexibility without code changes
- Can adapt to different shader languages

### Custom Suffix
- Some codebases may have naming conventions
- Allows matching project style
- Avoids conflicts with existing variables

### Integration in Preprocess
- One-stop solution for all shader preprocessing
- Consistent with other fixes in the pipeline
- Automatic for HLSL shaders

### Direct Call Option
- Allows fine-grained control
- Useful for testing specific cases
- Can be used standalone without full preprocessing

## Conclusion

This feature provides a **flexible, safe, and performant** way to handle reserved keywords used as variable names in shader code. It integrates seamlessly with the existing preprocessing pipeline while maintaining the option for direct use when needed.
