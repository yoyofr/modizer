#include "HLSLTypeFixer.h"
#include <iostream>

int main() {
    std::string testCode = R"(
float3 ret1, neu, musl, musr, screen3, pos0, pos1;
float2 pq, pq2;
float width, depth;
int mask1, mask2;
float n, dist2;
mask1 = (pq.x>=0)* (pq.x <=width);
mask2 = (pq2.x>=0)*(pq2.x <= depth);
)";

    std::cout << "Original code:\n" << testCode << "\n\n";
    
    HLSLTypeFixer fixer;
    fixer.setVerbose(true);
    
    std::string fixed = fixer.autoFix(testCode);
    
    std::cout << "\n\nFixed code:\n" << fixed << "\n";
    
    return 0;
}
