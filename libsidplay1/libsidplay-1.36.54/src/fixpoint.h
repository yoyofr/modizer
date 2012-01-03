/*******************************************************************************
*
*	File header:	PseudoFloat.h
*
*	Description:    Fixed point class
*
*
*	Author:         Vincent Penné
*
*	Date:           Sat 18th October 1997
*
*******************************************************************************/



/***************************************************************/
#ifndef SIDPLAY1_FIXPOINT_H
#define SIDPLAY1_FIXPOINT_H

class fixed {
public:
   int v;

   inline fixed() {}
   inline fixed(const int a, int dummy) { v=a; dummy=dummy; } // basic constructor (ugly !)

   inline fixed(const int a) { v=a<<16; }
   inline fixed(const unsigned int a) { v=a<<16; }
   inline fixed(const long a) { v=a<<16; }
   inline fixed(const unsigned long a) { v=a<<16; }
   inline fixed(double a) { v=int(a*(1<<16)); }
   inline fixed(float a) { v=int(a*(1<<16)); }

//   inline fixed make_fixed(int a) { fixed r; r.v=a; return r; }
//   inline fixed(fixed &a) { v=a.v; }
   inline operator float() { return float(v)/(1<<16); }
   inline operator double() { return double(v)/(1<<16); }
//   inline &operator fixed() { return make_fixed(a.v); }
//   inline fixed(int &a) { v=a<<16; }
//   inline fixed(float a) { v=int(a*(1<<16)); }
//   inline fixed(double a) { v=int(a*(1<<16)); }
   inline operator signed char() { return v>>16; }
   inline operator unsigned char() { return v>>16; }
   inline operator int() { return v>>16; }
   inline operator unsigned int() { return v>>16; }
   inline operator long() { return v>>16; }
   inline operator unsigned long() { return v>>16; }
   inline friend fixed operator - (const fixed &a) { return fixed(-a.v, 0); }
   static inline fixed multiply(const fixed &a, const fixed &b) { return fixed((a.v>>8)*(b.v>>8),0); }
   static inline fixed divide(const fixed &a, const fixed &b) { return fixed((a.v<<8)/(b.v>>8),0); }
   inline fixed operator *(const fixed &b) { return fixed((v>>8)*(b.v>>8),0); }
   inline fixed operator /(const fixed &b) { return fixed((v<<8)/(b.v>>8),0); }
   inline fixed operator +(const fixed &b) { return fixed(v+b.v,0); }
   inline fixed operator -(const fixed &b) { return fixed(v-b.v,0); }
   inline fixed& operator +=(const fixed &b) { v+=b.v; return *this; }
   inline fixed& operator -=(const fixed &b) { v-=b.v; return *this; }
   inline fixed& operator *=(const fixed &b) { v=(v>>8)*(b.v>>8); return *this; }
   inline fixed& operator *=(const int b) { v*=b; return *this; }
   inline fixed& operator *=(const double b) { *this*=fixed(b); return *this; }
   inline fixed& operator *=(const float b) { *this*=fixed(b); return *this; }
   inline friend float& operator *=(float &a, fixed &b) { a = a*float(b); return a; }
   inline fixed& operator /=(const fixed &b) { v=(v<<8)/(b.v>>8); return *this; }
   inline fixed& operator /=(const int b) { v/=b; return *this; }
   inline fixed& operator /=(const unsigned int b) { v/=b; return *this; }
   inline fixed& operator /=(const double b) { *this/=fixed(b); return *this; }
   inline fixed& operator /=(const float b) { *this/=fixed(b); return *this; }
   inline bool operator ==(const fixed &b) { return v==b.v; }
   inline bool operator !=(const fixed &b) { return v!=b.v; }
   inline bool operator !=(const float &b) { return v!=b/(1<<16); }
   inline bool operator <(const fixed &b) { return v<b.v; }
   inline bool operator <(double b) { return *this<fixed(b); }
   inline bool operator >(const fixed &b) { return v>b.v; }
   inline bool operator >(const int b) { return v>(b<<16); }
   inline bool operator <=(const fixed &b) { return v<=b.v; }
   inline bool operator >=(const fixed &b) { return v>=b.v; }

/*   friend inline fixed operator * (const double &a, const fixed &b)
      { return make_fixed((a*(1<<16))*b.v); }*/

   friend inline fixed operator / (const float a, const fixed &b)
        { return fixed(a)/b; }
   friend inline fixed operator / (const double a, const fixed &b)
        { return fixed(a)/b; }

    friend inline fixed operator + (const int a, const fixed &b)
      { return fixed((a<<16)+b.v,0); }
    friend inline fixed operator + (const double a, const fixed &b)
      { return fixed(int(a*(1<<16))+b.v,0); }
    friend inline fixed operator - (const double a, const fixed &b)
      { return fixed(int(a*(1<<16))-b.v,0); }
    friend inline fixed operator + (const float a, const fixed &b)
      { return fixed(int(a*(1<<16))+b.v,0); }
    friend inline fixed operator - (const float a, const fixed &b)
      { return fixed(int(a*(1<<16))-b.v,0); }

  friend inline fixed operator - (const fixed &a, const double b)
      { return fixed(a.v-int(b*(1<<16)),0); }
  friend inline fixed operator + (const fixed &a, const float b)
      { return fixed(a.v+int(b*(1<<16)),0); }

   friend inline fixed operator / (const int a, const fixed &b)
      { return fixed((a<<24)/(b.v>>8),0); }
   friend inline fixed operator / (const fixed &a, const int b)
      { return fixed(a.v/b,0); }
   friend inline fixed operator / (const fixed &a, const long int b)
      { return fixed(a.v/b,0); }

   friend inline fixed operator * (const int b, const fixed &a)
      { return fixed(a.v*b,0); }
   friend inline fixed operator * (const fixed &a, const int b)
      { return fixed(a.v*b,0); }
   friend inline fixed operator * (const fixed &a, const long int b)
      { return fixed(a.v*b,0); }


//   inline operator float() { return v/(1<<16); }
//   inline operator double() { return v/double(1<<16); }
};

inline fixed operator / (const fixed &a, const double b)
      { return fixed::divide(a,fixed(b)); }
inline fixed operator / (const fixed &a, const float b)
      { return fixed::divide(a,fixed(b)); }

inline fixed operator * (const fixed &a, const double b)
      { return fixed::multiply(a,fixed(b)); }
inline fixed operator * (const fixed &a, const float b)
      { return fixed::multiply(a,fixed(b)); }

/***************************************************************/
#endif  /* SIDPLAY1_FIXPOINT_H */
