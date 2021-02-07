#ifndef MISC_H
#define MISC_H

inline int Max(int x, int y) { return (x > y) ? x : y; }
inline int Min(int x, int y) { return (x < y) ? x : y; }
inline int Abs(int x) { return x >= 0 ? x : -x; }

inline int Limit(int v, int max, int min) 
{ 
	return v > max ? max : (v < min ? min : v); 
}

#endif // MISC_H

