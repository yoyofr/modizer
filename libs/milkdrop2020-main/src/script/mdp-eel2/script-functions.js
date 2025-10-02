

const g_closefact =  0.00001;

function NonZero(a)
{
    if (a === 0.0) return false;
    return Math.abs(a)>g_closefact;
}

function IsZero(a)
{
    if (a === 0.0) return true;
    return Math.abs(a)<g_closefact;
}

function IsTrue(a)
{
    return NonZero(a);
}

function fn_sin(f)
{
    return Math.sin(f);
}

function fn_cos(f)
{
    return Math.cos(f);
}

function fn_tan(f)
{
    return Math.tan(f);
}

function fn_asin(f)
{
    return Math.asin(f);
}

function fn_acos(f)
{
    return Math.acos(f);
}

function fn_atan(f)
{
    return Math.atan(f);
}

function fn_log(f)
{
    return Math.log(f);
}

function fn_log10(f)
{
    return Math.log10(f);
}

function fn_atan2(x, y)
{
    return Math.atan2(x, y);
}


function fn_pow(x, y)
{
    return Math.pow(x, y);
}


function fn_exp(x)
{
    return Math.exp(x);
}



function fn_floor(x)
{
    return Math.floor(x);
}

function fn_sqrt(x)
{
    return Math.sqrt(x);
}

function fn_invsqrt(x)
{
    return 1.0 / Math.sqrt(x);
}

function fn_sqr(x)
{
    return (x) * (x);
}

function fn_ceil(x)
{
    return Math.ceil(x);
}
function fn_abs(x)
{
    return Math.abs(x);
}

function fn_min(x, y)
{
    return Math.min(x, y);
}

function fn_max(x, y)
{
    return Math.max(x, y);
}

function fn_add(x, y)
{
    return x + y;
}

function fn_sub(x, y)
{
    return x - y;
}

function fn_mul(x, y)
{
    return x * y;
}

function fn_div(x, y)
{
    return x / y;
}

function fn__mod(x, y)
{
    //return Math.fmod(x, y);
    return x % y;
}

function ToInt(x)
{
    return Math.floor(x)|0;
}

function fn_bitwise_or(x, y)
{
    return (ToInt(x) | ToInt(y));
}

function fn_bitwise_and(x, y)
{
    return (ToInt(x) & ToInt(y));
}

function fn__uplus(x)
{
    return +x;
}

function fn__uminus(x)
{
    return -x;
}


function fn_sign(x)
{
    return Math.sign(x);
}




//---------------------------------------------------------------------------------------------------------------
function fn_int_rand(f)
{
    let x= Math.floor(f);
    if (x < 1.0) x=1.0;
    return Math.random() * x;
}

function fn_rand(x)
{
    return fn_int_rand(x);
}

function fn_band(a, b)
{
    return (IsTrue(a) && IsTrue(b)) ? 1 : 0;
}
function fn_bor(a, b)
{
    return (IsTrue(a) || IsTrue(b)) ? 1 : 0;
}

function fn_sigmoid(x, constraint)
{
    let t = (1+Math.exp(-x * (constraint)));
    return (t != 0.0) ? (1.0/t) : 0;
}

function fn__not(a)
{
    return !IsTrue(a) ? 1 : 0;
}

function fn__equal(a, b)
{
    if (a == b) return 1;
    return IsZero(a - b) ? 1 : 0;
}


function fn__noteq(a, b)
{
    if (a == b) return 0;
    return !IsZero(a - b) ? 1 : 0;
}

function fn__below(a, b)
{
    return (a < b) ? 1 : 0;
}

function fn__above(a, b)
{
    return (a > b) ? 1 : 0;
}

function fn__beleq(a, b)
{
    return (a <= b) ? 1 : 0;
}

function fn__aboeq(a, b)
{
    return (a >= b) ? 1 : 0;
}






//
g_kernel_map = [];
g_kernel_count = 1;

function kernel_register(source_code)
{
    let kernel = eval(source_code);
    let kid = g_kernel_count++;

    g_kernel_map[kid] = kernel;
    console.log("registered kernel", kid, source_code, kernel);
    return kid;
}

function kernel_execute(kid, addr, count)
{
    kernel = g_kernel_map[kid];
    kernel(addr, count);
}

