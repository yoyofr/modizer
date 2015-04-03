/*
 *  FFTAccelerate.cpp

 
 Copyright (C) 2012 Tom Hoddes
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FFTAccelerate.h"
#include <stdio.h>
#include <stdlib.h>
#include <Accelerate/Accelerate.h>
#include <vector>
#include <math.h>

void FFTAccelerate::doFFTReal(float samples[], float amp[], int numSamples)
{
	int i;
	vDSP_Length log2n = log2f(numSamples);
    int nOver2 = numSamples/2;
    //-- window
    //vDSP_blkman_window(window, windowSize, 0);
    vDSP_vmul(samples, 1, window, 1, in_real, 1, numSamples);

    //Convert float array of reals samples to COMPLEX_SPLIT array A
	vDSP_ctoz((COMPLEX*)in_real,2,&A,1,nOver2);

    //Perform FFT using fftSetup and A
    //Results are returned in A
	vDSP_fft_zrip(fftSetup, &A, 1, log2n, FFT_FORWARD);
    
    // scale by 1/2*n because vDSP_fft_zrip doesn't use the right scaling factors natively ("for better performances")
    {
        const float scale = 1.0f/(2.0f*(float)numSamples);
        vDSP_vsmul( A.realp, 1, &scale, A.realp, 1, numSamples/2 );
        vDSP_vsmul( A.imagp, 1, &scale, A.imagp, 1, numSamples/2 );
    }
    
    //Convert COMPLEX_SPLIT A result to float array to be returned
    /*amp[0] = A.realp[0]/(numSamples*2);
	for(i=1;i<numSamples/2;i++)
		amp[i]=sqrt(A.realp[i]*A.realp[i]+A.imagp[i]*A.imagp[i]);*/
    
    // collapse split complex array into a real array.
    // split[0] contains the DC, and the values we're interested in are split[1] to split[len/2] (since the rest are complex conjugates)
    vDSP_zvabs( &A, 1, amp, 1, numSamples/2 );
}

//Constructor
FFTAccelerate::FFTAccelerate (int numSamples)
{
	vDSP_Length log2n = log2f(numSamples);
	fftSetup = vDSP_create_fftsetup(log2n, FFT_RADIX2);
	int nOver2 = numSamples/2;
	A.realp = (float *) malloc(nOver2*sizeof(float));
	A.imagp = (float *) malloc(nOver2*sizeof(float));
    window = (float *) malloc(numSamples * sizeof(float));
    in_real = (float *) malloc(numSamples * sizeof(float));
    memset(window, 0, numSamples * sizeof(float));
    vDSP_hann_window(window, numSamples, vDSP_HANN_NORM);
}


//Destructor
FFTAccelerate::~FFTAccelerate ()
{
    free(in_real);
    free(window);
	free(A.realp);
	free(A.imagp);
    vDSP_destroy_fftsetup(fftSetup);
}

