#include "pfc.h"
#ifdef _MSC_VER
#include <intrin.h>
#include <assert.h>
#endif
#ifndef _MSC_VER
#include <signal.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include <math.h>

namespace pfc {
	bool permutation_is_valid(t_size const * order, t_size count) {
		bit_array_bittable found(count);
		for(t_size walk = 0; walk < count; ++walk) {
			if (order[walk] >= count) return false;
			if (found[walk]) return false;
			found.set(walk,true);
		}
		return true;
	}
	void permutation_validate(t_size const * order, t_size count) {
		if (!permutation_is_valid(order,count)) throw exception_invalid_permutation();
	}

	t_size permutation_find_reverse(t_size const * order, t_size count, t_size value) {
		if (value >= count) return ~0;
		for(t_size walk = 0; walk < count; ++walk) {
			if (order[walk] == value) return walk;
		}
		return ~0;
	}

	void create_move_items_permutation(t_size * p_output,t_size p_count,const bit_array & p_selection,int p_delta) {
		t_size * const order = p_output;
		const t_size count = p_count;

		pfc::array_t<bool> selection; selection.set_size(p_count);
		
		for(t_size walk = 0; walk < count; ++walk) {
			order[walk] = walk;
			selection[walk] = p_selection[walk];
		}

		if (p_delta<0)
		{
			for(;p_delta<0;p_delta++)
			{
				t_size idx;
				for(idx=1;idx<count;idx++)
				{
					if (selection[idx] && !selection[idx-1])
					{
						pfc::swap_t(order[idx],order[idx-1]);
						pfc::swap_t(selection[idx],selection[idx-1]);
					}
				}
			}
		}
		else
		{
			for(;p_delta>0;p_delta--)
			{
				t_size idx;
				for(idx=count-2;(int)idx>=0;idx--)
				{
					if (selection[idx] && !selection[idx+1])
					{
						pfc::swap_t(order[idx],order[idx+1]);
						pfc::swap_t(selection[idx],selection[idx+1]);
					}
				}
			}
		}
	}
}

void order_helper::g_swap(t_size * data,t_size ptr1,t_size ptr2)
{
	t_size temp = data[ptr1];
	data[ptr1] = data[ptr2];
	data[ptr2] = temp;
}


t_size order_helper::g_find_reverse(const t_size * order,t_size val)
{
	t_size prev = val, next = order[val];
	while(next != val)
	{
		prev = next;
		next = order[next];
	}
	return prev;
}


void order_helper::g_reverse(t_size * order,t_size base,t_size count)
{
	t_size max = count>>1;
	t_size n;
	t_size base2 = base+count-1;
	for(n=0;n<max;n++)
		g_swap(order,base+n,base2-n);
}


void pfc::crash() {
#ifdef _MSC_VER
	__debugbreak();
#else

#if defined(__ANDROID__) && PFC_DEBUG
	nixSleep(1);
#endif

    raise(SIGINT);
#endif
}


void pfc::byteswap_raw(void * p_buffer,const t_size p_bytes) {
	t_uint8 * ptr = (t_uint8*)p_buffer;
	t_size n;
	for(n=0;n<p_bytes>>1;n++) swap_t(ptr[n],ptr[p_bytes-n-1]);
}

void pfc::outputDebugLine(const char * msg) {
#ifdef _WIN32
	OutputDebugString(pfc::stringcvt::string_os_from_utf8(PFC_string_formatter() << msg << "\n") );
#elif defined(__ANDROID__)
	__android_log_write(ANDROID_LOG_INFO, "Debug", msg);
#else
	printf("%s\n", msg);
#endif
}

#if PFC_DEBUG

#ifdef _WIN32
void pfc::myassert_win32(const wchar_t * _Message, const wchar_t *_File, unsigned _Line) {
	if (IsDebuggerPresent()) pfc::crash();
	_wassert(_Message,_File,_Line);
}
#else

void pfc::myassert(const char * _Message, const char *_File, unsigned _Line)
{
	PFC_DEBUGLOG << "Assert failure: \"" << _Message << "\" in: " << _File << " line " << _Line;
	crash();
}
#endif

#endif


t_uint64 pfc::pow_int(t_uint64 base, t_uint64 exp) {
	t_uint64 mul = base;
	t_uint64 val = 1;
	t_uint64 mask = 1;
	while(exp != 0) {
		if (exp & mask) {
			val *= mul;
			exp ^= mask;
		}
		mul = mul * mul;
		mask <<= 1;
	}
	return val;
}

double pfc::exp_int( const double base, const int expS ) {
    //    return pow(base, (double)v);
    
    bool neg;
    unsigned exp;
    if (expS < 0) {
        neg = true;
        exp = (unsigned) -expS;
    } else {
        neg = false;
        exp = (unsigned) expS;
    }
    double v = 1.0;
    if (true) {
        if (exp) {
            double mul = base;
            for(;;) {
                if (exp & 1) v *= mul;
                exp >>= 1;
                if (exp == 0) break;
                mul *= mul;
            }
        }
    } else {
        for(unsigned i = 0; i < exp; ++i) {
            v *= base;
        }
    }
    if (neg) v = 1.0 / v;
    return v;
}


t_int32 pfc::rint32(double p_val) { return (t_int32)floor(p_val + 0.5); }
t_int64 pfc::rint64(double p_val) { return (t_int64)floor(p_val + 0.5); }
