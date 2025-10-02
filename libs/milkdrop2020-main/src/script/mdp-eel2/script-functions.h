
#pragma once

#include "../script.h"


extern int g_FuncDebug;

namespace Script { namespace mdpx {
    
#define USE_HIGH_PRECISION_MATH (0)

#define FUNC_DECL inline
#define _IF(_test, _then, _else)  (IsTrue(_test) ? (_then) : (_else))
#define _LOGICAL_AND(_a, _b)      ((IsTrue(_a) && IsTrue(_b)) ? ValueTrue : ValueFalse)
#define _LOGICAL_OR(_a, _b)       ((IsTrue(_a) || IsTrue(_b)) ? ValueTrue : ValueFalse)


    namespace Functions
    {
        
        typedef ValueType (*OpFunc0)();
        typedef ValueType (*OpFunc1)(ValueType);
        typedef ValueType (*OpFunc2)(ValueType, ValueType);
        typedef ValueType (*OpFunc3)(ValueType, ValueType, ValueType);

        
        static constexpr ValueType ValueTrue  = 1.0;
        static constexpr ValueType ValueFalse = 0.0;

        static constexpr ValueType g_closefact =  0.00001;
        
        FUNC_DECL bool NonZero(ValueType a)
        {
            if (a == 0.0) return false;
            return fabs(a)>g_closefact;
        }
        
        FUNC_DECL bool IsZero(ValueType a)
        {
            if (a == 0.0) return true;
            return fabs(a)<g_closefact;
        }
        

        FUNC_DECL bool IsTrue(ValueType a)
        {
            return NonZero(a);
        }
        
        FUNC_DECL bool IsEqual(ValueType a, ValueType b)
        {
            if (a == b) return true;
            return IsZero(a - b);
        }
    
        FUNC_DECL ValueType fn_checked(ValueType x)
        {
            if (isfinite(x)) {
                return x;
            } else {
                return 0.0;
            }
        }
    
        FUNC_DECL ValueType fn_assign(ValueType &target, ValueType x)
        {
            if (isfinite(x)) {
                target = x;
            } else {
                target = 0.0;
            }
            return x; // return unchecked value
        }
    

        FUNC_DECL ValueType fn_istrue(ValueType a)
        {
            return NonZero(a);
        }



        
#if USE_HIGH_PRECISION_MATH
        FUNC_DECL ValueType fn_sin(ValueType f)
        {
            return sin(f);
        }
        
        FUNC_DECL ValueType fn_cos(ValueType f)
        {
            return cos(f);
        }
        
        FUNC_DECL ValueType fn_tan(ValueType f)
        {
            return tan(f);
        }
        
        FUNC_DECL ValueType fn_asin(ValueType f)
        {
            return asin(f);
        }
        
        FUNC_DECL ValueType fn_acos(ValueType f)
        {
            return acos(f);
        }
        
        FUNC_DECL ValueType fn_atan(ValueType f)
        {
            return atan(f);
        }
        
        FUNC_DECL ValueType fn_log(ValueType f)
        {
            return log(f);
        }
        
        FUNC_DECL ValueType fn_log10(ValueType f)
        {
            return log10(f);
        }
        
        FUNC_DECL ValueType fn_atan2(ValueType x, ValueType y)
        {
            return atan2(x, y);
        }
        
        
        FUNC_DECL ValueType fn_pow(ValueType x, ValueType y)
        {
            return pow(x, y);
        }
        
        
        FUNC_DECL ValueType fn_exp(ValueType x)
        {
            return exp(x);
        }

#else
        
        FUNC_DECL ValueType fn_sin(ValueType f)
        {
            return (ValueType)sinf((float)f);
        }
        
        FUNC_DECL ValueType fn_cos(ValueType f)
        {
            return (ValueType)cosf((float)f);
        }
        
        FUNC_DECL ValueType fn_tan(ValueType f)
        {
            return (ValueType)tanf((float)f);
        }
        
        FUNC_DECL ValueType fn_asin(ValueType f)
        {
            return asinf((float)f);
        }
        
        FUNC_DECL ValueType fn_acos(ValueType f)
        {
            return acosf((float)f);
        }
        
        FUNC_DECL ValueType fn_atan(ValueType f)
        {
            return atanf((float)f);
        }
        
        FUNC_DECL ValueType fn_log(ValueType f)
        {
            return logf((float)f);
        }
        
        FUNC_DECL ValueType fn_log10(ValueType f)
        {
            return log10f((float)f);
        }
        
        FUNC_DECL ValueType fn_atan2(ValueType x, ValueType y)
        {
            return atan2f((float)x, (float)y);
        }
        
        
        FUNC_DECL ValueType fn_pow(ValueType x, ValueType y)
        {
            return powf((float)x, (float)y);
        }
        
        
        FUNC_DECL ValueType fn_exp(ValueType x)
        {
            return expf((float)x);
        }

        
#endif
        
        
        
        FUNC_DECL ValueType fn_floor(ValueType x)
        {
            return floor(x);
        }
        
        FUNC_DECL ValueType fn_sqrt(ValueType x)
        {
            return sqrt( fabs(x) );
        }
        
        FUNC_DECL ValueType fn_invsqrt(ValueType x)
        {
            return 1.0 / sqrt( fabs(x) );
        }
        
        FUNC_DECL ValueType fn_sqr(ValueType x)
        {
            return (x) * (x);
        }
        
        FUNC_DECL ValueType fn_ceil(ValueType x)
        {
            return ceil(x);
        }
        FUNC_DECL ValueType fn_abs(ValueType x)
        {
            return fabs(x);
        }
        
        FUNC_DECL ValueType fn_min(ValueType x, ValueType y)
        {
            return fmin(x, y);
        }
        
        FUNC_DECL ValueType fn_max(ValueType x, ValueType y)
        {
            return fmax(x, y);
        }
        
        FUNC_DECL ValueType fn_add(ValueType x, ValueType y)
        {
            return x + y;
        }
        
        FUNC_DECL ValueType fn_sub(ValueType x, ValueType y)
        {
            return x - y;
        }
        
        FUNC_DECL ValueType fn_mul(ValueType x, ValueType y)
        {
            return x * y;
        }
        
        FUNC_DECL ValueType fn_div(ValueType x, ValueType y)
        {
            return x / y;
        }
        
    
    
        
        FUNC_DECL int ToInt(ValueType x)
        {
            return (int)fn_checked(x);
        }

        FUNC_DECL uint32_t ToUInt32(ValueType x)
        {
            return (uint32_t)fn_checked(x);
        }

     
        FUNC_DECL ValueType fn__mod(ValueType x, ValueType y)
        {
            // integer mod
            
            int ix = ToInt(x);
            uint32_t iy = (uint32_t)fabs(y);
            
            ValueType result = 0.0;
            if (iy != 0)
            {
                int mod = ix % iy; // integral mod
                result = (ValueType)mod;
            }
            return result;
        }

        
        FUNC_DECL ValueType fn_bitwise_or(ValueType x, ValueType y)
        {
            return (ValueType)(ToInt(x) | ToInt(y));
        }
        
        FUNC_DECL ValueType fn_bitwise_and(ValueType x, ValueType y)
        {
            return (ValueType)(ToInt(x) & ToInt(y));
        }
        
        FUNC_DECL ValueType fn__uplus(ValueType x)
        {
            return +x;
        }
        
        FUNC_DECL ValueType fn__uminus(ValueType x)
        {
            return -x;
        }
        
        
        FUNC_DECL ValueType fn_sign(ValueType x)
        {
            if (x < 0.0) return -1.0;
            if (x > 0.0) return  1.0;
            return 0.0;
        }
        
        
        
        
        //---------------------------------------------------------------------------------------------------------------
    
    
        static int fn_rand_r(unsigned int *seed)
        {
            return ((*seed = *seed * 1103515245 + 12345) & RAND_MAX);
        }

        FUNC_DECL ValueType fn_rand(ValueType &state, ValueType f)
        {
            // get random value
            uint32_t rstate = (uint32_t)state;
            int r = fn_rand_r(&rstate);
            state = (ValueType)rstate;
            
            ValueType x=floor(f);
            if (x < 1.0) x=1.0;
            ValueType result = (ValueType) (r*(1.0/(double)(RAND_MAX))*x);
            
            
            return result;
        }

        FUNC_DECL ValueType fn_band(ValueType a, ValueType b)
        {
            return (IsTrue(a) && IsTrue(b)) ? ValueTrue : ValueFalse;
        }
        FUNC_DECL ValueType fn_bor(ValueType a, ValueType b)
        {
            return (IsTrue(a) || IsTrue(b)) ? ValueTrue : ValueFalse;
        }
        
        FUNC_DECL ValueType fn_sigmoid(ValueType x, ValueType constraint)
        {
            ValueType t = (1+exp(-x * (constraint)));
            return (t != 0.0) ? (1.0/t) : 0;
        }
        
        FUNC_DECL ValueType fn__not(ValueType a)
        {
            return !IsTrue(a) ? ValueTrue : ValueFalse;
        }
        
        FUNC_DECL ValueType fn__equal(ValueType a, ValueType b)
        {
            return IsEqual(a, b) ? ValueTrue : ValueFalse;
        }
        
        
        FUNC_DECL ValueType fn__noteq(ValueType a, ValueType b)
        {
            return !IsEqual(a, b) ? ValueTrue : ValueFalse;
        }
        
        FUNC_DECL ValueType fn__below(ValueType a, ValueType b)
        {
            return (a < b) ? ValueTrue : ValueFalse;
        }
        
        FUNC_DECL ValueType fn__above(ValueType a, ValueType b)
        {
            return (a > b) ? ValueTrue : ValueFalse;
        }
        
        FUNC_DECL ValueType fn__beleq(ValueType a, ValueType b)
        {
            return (a <= b) ? ValueTrue : ValueFalse;
        }
        
        FUNC_DECL ValueType fn__aboeq(ValueType a, ValueType b)
        {
            return (a >= b) ? ValueTrue : ValueFalse;
        }
        
    
        FUNC_DECL ValueType fn_assert_equals(ValueType a, ValueType b)
        {
            assert(a == b);
            return 0;
        }


        
        
    }
    
}}

