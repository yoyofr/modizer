# Keyword Renaming in ShaderPreprocessor

## Overview

The `renameKeywordsAsVariables` method allows you to automatically rename reserved keywords when they are used as variable names in shader code. This is particularly useful when processing HLSL shaders that may use keywords like `sample` as variable names.

## Features

- **Generic keyword list**: You can specify any list of keywords to rename, not just hardcoded ones
- **Smart detection**: Only renames keywords when they're actually used as variables (with type declarations)
- **Scope-aware**: Renames all usages of the variable after its declaration
- **Safe**: Won't rename function names, function parameters, or other non-variable usages

## Usage

### Basic Usage (via preprocess method)

When using the `preprocess()` method with HLSL language mode, the preprocessor automatically renames the `sample` keyword:

```cpp
ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
std::string result = preprocessor.preprocess(shaderCode);
```

**Input:**
```hlsl
float3 sample = tex2D(sampler_main, uv);
ret = sample*sample*sample;
```

**Output:**
```hlsl
float3 sample_var = tex2D(sampler_main, uv);
ret = sample_var*sample_var*sample_var;
```

### Direct Usage with Custom Keywords

You can also call `renameKeywordsAsVariables` directly with your own list of keywords:

```cpp
ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);

// Define keywords to rename
std::vector<std::string> keywords = {"sample", "input", "output", "register"};

// Rename with custom suffix
std::string result = preprocessor.renameKeywordsAsVariables(
    shaderCode, 
    keywords, 
    "_var"  // Optional suffix, defaults to "_var"
);
```

### Custom Suffix

You can specify a custom suffix instead of the default `_var`:

```cpp
std::vector<std::string> keywords = {"sample"};
std::string result = preprocessor.renameKeywordsAsVariables(
    shaderCode, 
    keywords, 
    "_renamed"  // Custom suffix
);
```

**Output:**
```hlsl
float3 sample_renamed = tex2D(sampler_main, uv);
ret = sample_renamed*sample_renamed*sample_renamed;
```

## How It Works

1. **Detection Phase**: The method scans the shader code for variable declarations that use one of the specified keywords:
   - Looks for pattern: `<type> <keyword> = ...;`
   - Only matches when keyword is used as a variable name (whole word match)
   - Detects the full declaration including the semicolon

2. **Renaming Phase**: For each detected usage:
   - Renames the variable in its declaration
   - Finds and renames all subsequent usages of that variable
   - Uses whole-word matching to avoid partial replacements
   - Processes declarations in reverse order to maintain correct string indices

## What Gets Renamed

✅ **Will be renamed:**
- Variable declarations: `float3 sample = ...;`
- All usages after the declaration: `sample * 2.0`, `sample.xyz`, `sample[0]`

❌ **Will NOT be renamed:**
- Function names: `float3 sample(float2 uv) { ... }`
- Function calls: `result = sample(uv);`
- Keywords in comments or strings
- Keywords that aren't preceded by a type name

## Integration with Preprocess Pipeline

The keyword renaming is automatically integrated into the `preprocess()` method as Step 5:

1. Remove invalid functions
2. Fix variable shadowing
3. Fix division by zero
4. Clean preprocessor directives
5. **Rename keywords used as variables** ← New step
6. Fix complex for loops

The keyword renaming is only applied when `m_language == ShaderLanguage::HLSL`.

## Common HLSL Keywords to Rename

Here are some HLSL keywords that might be used as variable names:

```cpp
std::vector<std::string> commonHLSLKeywords = {
    "sample",      // Commonly used for texture samples
    "input",       // Input data
    "output",      // Output data
    "register",    // Register allocation
    "texture",     // Texture variables
    "buffer",      // Buffer variables
};
```

## API Reference

### Method Signature

```cpp
std::string renameKeywordsAsVariables(
    const std::string& shaderSource,
    const std::vector<std::string>& keywords,
    const std::string& suffix = "_var"
);
```

**Parameters:**
- `shaderSource`: The shader source code to process
- `keywords`: Vector of keywords to rename when used as variables
- `suffix`: Suffix to append to renamed variables (default: `"_var"`)

**Returns:**
- Processed shader source with renamed variables

**Complexity:**
- Time: O(n × k × t) where n = source length, k = keyword count, t = type count
- Space: O(n) for result string

## Examples

### Example 1: Single Keyword

```cpp
ShaderPreprocessor preprocessor(ShaderLanguage::HLSL);
std::vector<std::string> keywords = {"sample"};

std::string input = R"(
float4 main(float2 uv : TEXCOORD) : COLOR {
    float3 sample = tex2D(sampler_main, uv);
    return float4(sample, 1.0);
}
)";

std::string output = preprocessor.renameKeywordsAsVariables(input, keywords);
// Result: sample becomes sample_var
```

### Example 2: Multiple Keywords

```cpp
std::vector<std::string> keywords = {"input", "output", "sample"};

std::string input = R"(
float4 process(float2 uv : TEXCOORD) : COLOR {
    float3 input = tex2D(sampler_input, uv);
    float3 sample = input * 2.0;
    float3 output = sample + input;
    return float4(output, 1.0);
}
)";

std::string result = preprocessor.renameKeywordsAsVariables(input, keywords);
// Result: input → input_var, sample → sample_var, output → output_var
```

### Example 3: Custom Suffix

```cpp
std::vector<std::string> keywords = {"sample"};

std::string result = preprocessor.renameKeywordsAsVariables(
    input, 
    keywords, 
    "_custom"
);
// Result: sample → sample_custom
```

## Notes

- The method is safe to call multiple times on the same code
- Works with all shader types (float, float2, float3, float4, int, etc.)
- Handles nested scopes correctly
- Performance optimized with pre-allocated vectors and efficient string operations
- Compatible with the existing preprocessing pipeline
