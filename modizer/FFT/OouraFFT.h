 /*
Copyright (c) 2009 Alex Wiltschko

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

//
//  OouraFFT.h
//  oScope
//
//  Created by Alex Wiltschko on 10/14/09.
//  This software is free because I love you.
//	But remember: Prof Ooura did the hard work.
//	A metaphor: Ooura raised the cow, slaughtered it, ground the beef,
//	made the beef sausages, and delivered it to my fridge.
//	I took it out of the fridge, microwaved it, 
//	slathered it with mustard and then called it dinner.
//
// Algorithm from fft4g.c
//
// Quirks and to-do's...
// * Rewrite to use single-precision (iPhone's single-precision abilities are more greaterer and betterest)
//	 - i'm not entirely familiar with the bit-reversal part of the FFT algorithm, so it might be a difficult rewrite

// data[2*k] = R[k], 0<=k<n/2, real frequency data
// data[2*k+1] = I[k], 0<k<n/2, imaginary frequency data
// data[1] = R[n/2]



#import <Foundation/Foundation.h>
#import "fft4g.h"
//#import "fft.h"

// Define integer values for various windows
// NOTE: not using any windows that require parameter input.
#define kWindow_Rectangular	1
#define kWindow_Hamming		2
#define kWindow_Hann		3
#define kWindow_Bartlett	4
#define kWindow_Triangular	5



@interface OouraFFT : NSObject {
	double *inputData;
	
	int dataLength;
	int numFrequencies;
	BOOL dataIsFrequency;


	float *win;
	int windowType;
	float *spectrumData;
	float **oldSpectrumData;
	int *beatDetected;
	float beatThreshold;
	// TODO: hold onto real and imaginary parts? no? maybe?

	// Two arrays that the FFT algorithm requires.
	// You won't interact with these outside of this class,
	// or even outside of a call to the FFT algorithm.
	int *fft_ip; // This is scratch space. 
	double *fft_w; // cos/sin table. 
	
	
}

@property double *inputData;

@property float *win;
@property int windowType;
@property int *beatDetected;
@property int numFrequencies;
@property int dataLength;
@property float *spectrumData;
@property float **oldSpectrumData;
@property float beatThreshold;

- (void)doFFT;
- (void)doIFFT;

- (void)calculateWelchPeriodogramWithNewSignalSegment;
- (void)windowSignalSegment;

- (id)initForSignalsOfLength:(int)numPoints;
- (BOOL)checkDataLength:(int)inDataLength;

- (void)initWindowType;

@end