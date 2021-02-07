#ifndef SPLINE_H
#define SPLINE_H

namespace reSIDfp
{

typedef float (*Params)[6];

class Spline {

private:
	const int paramsLength;
	Params params;
	float* c;

public:
	Spline(const float input[][2], const int inputLength);
	~Spline() { delete [] params; }

	void evaluate(const float x, float* out);
};

} // namespace reSIDfp

#endif
