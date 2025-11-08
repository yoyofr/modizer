#include "HLSLTypeFixer.h"
#include <iostream>
#include <string>

int main() {
    HLSLTypeFixer fixer;
    fixer.setVerbose(true);
    
    // Test case 1: Original problem - division followed by multiplication
    std::string testCode1 = R"(
int anz = 3;
float n = 0;
float time;
while (n <= anz) {
    dist = 1-frac(1.0/anz*n+time/2);
    n++;
}
)";
    
    std::cout << "=== Test Case 1: Division followed by multiplication ===" << std::endl;
    std::cout << "Original code:" << std::endl;
    std::cout << testCode1 << std::endl;
    
    std::string fixed1 = fixer.autoFix(testCode1);
    std::cout << "\nFixed code:" << std::endl;
    std::cout << fixed1 << std::endl;
    
    // Test case 2: Simple division without multiplication
    std::string testCode2 = R"(
int anz = 3;
float result = 1.0 / anz;
)";
    
    std::cout << "\n=== Test Case 2: Simple division ===" << std::endl;
    std::cout << "Original code:" << std::endl;
    std::cout << testCode2 << std::endl;
    
    std::string fixed2 = fixer.autoFix(testCode2);
    std::cout << "\nFixed code:" << std::endl;
    std::cout << fixed2 << std::endl;
    
    // Test case 3: Integer literal divided by integer variable
    std::string testCode3 = R"(
int anz = 3;
float result = 10 / anz * 2;
)";
    
    std::cout << "\n=== Test Case 3: Integer literal / int variable * literal ===" << std::endl;
    std::cout << "Original code:" << std::endl;
    std::cout << testCode3 << std::endl;
    
    std::string fixed3 = fixer.autoFix(testCode3);
    std::cout << "\nFixed code:" << std::endl;
    std::cout << fixed3 << std::endl;
    
    // Test case 4: Multiple divisions
    std::string testCode4 = R"(
int a = 5;
int b = 10;
float x = 1.0 / a * 3 + 2.0 / b;
)";
    
    std::cout << "\n=== Test Case 4: Multiple divisions ===" << std::endl;
    std::cout << "Original code:" << std::endl;
    std::cout << testCode4 << std::endl;
    
    std::string fixed4 = fixer.autoFix(testCode4);
    std::cout << "\nFixed code:" << std::endl;
    std::cout << fixed4 << std::endl;
    
    // Test case 5: Division in complex expression
    std::string testCode5 = R"(
int anz = 3;
float time = 1.5;
float n = 2.0;
float dist = 1.0 - frac(1.0/anz*n + time/2);
)";
    
    std::cout << "\n=== Test Case 5: Complex expression ===" << std::endl;
    std::cout << "Original code:" << std::endl;
    std::cout << testCode5 << std::endl;
    
    std::string fixed5 = fixer.autoFix(testCode5);
    std::cout << "\nFixed code:" << std::endl;
    std::cout << fixed5 << std::endl;
    
    return 0;
}
