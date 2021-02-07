#ifndef ___FLOATONLY_H_
#define ___FLOATONLY_H_

#include <math.h>

//#define float float
#define sin sinf
#define cos cosf
#define sqrt sqrtf
#define atan2 atan2f
#define floor floorf
#define ceil ceilf
#define pow powf
#define tan tanf
#define atan atanf

#define tanh tanhf
#define frexp frexpf
#define modf modff
#define fabs fabsf
#define acos acosf
#define asin asinf
#define cosh coshf
#define sinh sinh
#define exp expf
#define ldexp ldexpf
#define log logf
#define log10 log10f
#define fmod fmodf
#define infinity infinityf
#define nan nanf
#define isnan isnanf
#define isinf isinff
#define finite finitef
#define copysign copysignf
#define ilogb ilogbf
#define asinh asinhf
#define cbrt cbrtf
#define nextafter nextafterf
#define rint rintf
#define scalbn scalbnf
#define tgamma tgammaf
#define nearbyint nearbyintf
#define lrint lrintf
#define round roundf
#define lround lroundf
#define trunc truncf
#define remquo remquof
#define copysign copysignf
#define fdim fdimf
#define fmax fmaxf
#define fmin fminf
#define fma fmaf
#define sincos sincosf
#define log1p log1pf
#define expm1 expm1f
#define acosh acoshf
#define atanh atanhf
#define remainder remainderf
#define gamma gammaf
#define lgamma lgammaf
#define erf erff
#define erfc erfcf
#define y0 y0f
#define y1 y1f
#define yn ynf
#define j0 j0f
#define j1 j1f
#define jn jnf
#define hypot hypotf
#define cabs cabsf
#define drem dremf

#undef MAXFLOAT
#undef M_E
#undef M_LOG2E
#undef M_LOG10E
#undef M_LN2
#undef M_LN10
#undef M_PI
#undef M_TWOPI
#undef M_PI_2
#undef M_PI_4
#undef M_3PI_4
#undef M_SQRTPI
#undef M_1_PI
#undef M_2_PI
#undef M_2_SQRTPI
#undef M_SQRT2
#undef M_SQRT1_2
#undef M_LN2LO
#undef M_LN2HI
#undef M_SQRT3
#undef M_IVLN10
#undef M_LOG2_E
#undef M_INVLN2

#define MAXFLOAT 3.40282347e+38f
#define M_E      2.7182818284590452354f
#define M_LOG2E  1.4426950408889634074f
#define M_LOG10E 0.43429448190325182765f
#define M_LN2    0.69314718055994530942f
#define M_LN10   2.30258509299404568402f
#define M_PI     3.14159265358979323846f
#define M_TWOPI       (M_PI * 2.0f)
#define M_PI_2      1.57079632679489661923f
#define M_PI_4      0.78539816339744830962f
#define M_3PI_4     2.3561944901923448370E0f
#define M_SQRTPI    1.77245385090551602792981f
#define M_1_PI      0.31830988618379067154f
#define M_2_PI      0.63661977236758134308f
#define M_2_SQRTPI  1.12837916709551257390f
#define M_SQRT2     1.41421356237309504880f
#define M_SQRT1_2   0.70710678118654752440f
#define M_LN2LO     1.9082149292705877000E-10f
#define M_LN2HI     6.9314718036912381649E-1f
#define M_SQRT3     1.73205080756887719000f
#define M_IVLN10    0.43429448190325182765f // 1 / log(10)
#define M_LOG2_E    0.693147180559945309417f
#define M_INVLN2    1.4426950408889633870E0f  // 1 / log(2) 


#endif /* ___FLOATONLY_H_ */
