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
//  OouraFFT.m
//  oScope
//

#import "OouraFFT.h"
#define MAX_INT 32767

#define BEAT_FFT_HISTORY_RANGE 8
#define BEAT_THRESHOLD 2.0f


// dataIsFrequency should be private to this class.
@interface OouraFFT () 
@property BOOL dataIsFrequency;
@end


@implementation OouraFFT

@synthesize inputData;
@synthesize dataLength;
@synthesize numFrequencies;
@synthesize dataIsFrequency;

@synthesize spectrumData;
@synthesize oldSpectrumData;
@synthesize win;
@synthesize windowType;
@synthesize beatDetected;
@synthesize beatThreshold;

# pragma mark - Visible Objective-C Methods

- (id)initForSignalsOfLength:(int)numPoints  {
	if ((self = [super init])) {

		//[self checkDataLength:numPoints];
		self.dataLength = numPoints;
		self.numFrequencies = numPoints/2;
		
		// Initialize helper arrays the FFT algorithm requires.
		fft_ip = (int *)malloc((2 + sqrt(numFrequencies)) * sizeof(int));
		fft_w  = (double *)malloc((numFrequencies ) * sizeof(double));
				
		fft_ip[0] = 0; // Need to set this before the first time we run the FFT
		
		// Initialize the raw input and output data arrays
		// In use, when calculating periodograms, outputData will not generally
		// be used. Look to realFrequencyData for that.
		self.inputData = (double *)malloc(self.dataLength*sizeof(double));
		memset(self.inputData,0,dataLength*sizeof(double));
		
		// Initialize the data we'd actually display
		self.spectrumData = (float *)malloc(self.numFrequencies*sizeof(float));
		memset(self.spectrumData,0,numFrequencies*sizeof(float));
		
		self.beatDetected = (int *)malloc(numFrequencies*sizeof(int));
		memset(self.beatDetected,0,numFrequencies*sizeof(int));
		
		beatThreshold=BEAT_THRESHOLD;
		
		self.oldSpectrumData = (float **)malloc(BEAT_FFT_HISTORY_RANGE*sizeof(float*));
		for (int i=0;i<BEAT_FFT_HISTORY_RANGE;i++) {
			self.oldSpectrumData[i] = (float *)malloc(self.numFrequencies*sizeof(float));
			for (int j=0;j<numFrequencies;j++) oldSpectrumData[i][j]=1000000.0f;
		}
								
		// Allocate the windowing function
		self.win = (float *)malloc(self.dataLength*sizeof(float));
		memset(self.win,0,self.dataLength*sizeof(float));
		
		// And then set the window type with our custom setter.
		self.windowType = kWindow_Hamming;
		
		[self initWindowType];
		
		// Start the oldestDataIndex at 0. 
		// This'll track which data needs to be cycled out and forgotten.
//		self.oldestDataIndex = 0; 
	}
	
	return self;
}


- (void)doFFT {
	rdft(self.dataLength, 1, self.inputData, fft_ip, fft_w);
}

- (void)doIFFT {
	rdft(self.dataLength, -1, self.inputData, fft_ip, fft_w);
	self.dataIsFrequency = NO;
}


- (void)calculateWelchPeriodogramWithNewSignalSegment {	
	float fft_sum;
	float modulus;
	// Apply a window to the signal segment
	[self windowSignalSegment];

	
	// Do the FFT
	rdft(self.dataLength, 1, self.inputData, fft_ip, fft_w);
	
	for (int i=0;i<BEAT_FFT_HISTORY_RANGE-1;i++) {
		memcpy(oldSpectrumData[i],oldSpectrumData[i+1],numFrequencies*sizeof(float));
	}
	
	// Get the real modulus of the FFT, and scale it
	for (int i=0; i<self.numFrequencies; i++) {
		modulus = sqrt(self.inputData[i*2]*self.inputData[i*2] + self.inputData[i*2+1]*self.inputData[i*2+1]);
		
		oldSpectrumData[BEAT_FFT_HISTORY_RANGE-1][i]=modulus;
		
		if (self.spectrumData[i]<modulus) self.spectrumData[i]=modulus;
		else self.spectrumData[i]*=0.8f;
		
		fft_sum=0;
		for (int j=0;j<BEAT_FFT_HISTORY_RANGE;j++) fft_sum=fft_sum+oldSpectrumData[j][i];
		if (oldSpectrumData[BEAT_FFT_HISTORY_RANGE-2][i]>(fft_sum*beatThreshold/BEAT_FFT_HISTORY_RANGE)) {
            if ((oldSpectrumData[BEAT_FFT_HISTORY_RANGE-2][i]>=oldSpectrumData[BEAT_FFT_HISTORY_RANGE-3][i])&&(oldSpectrumData[BEAT_FFT_HISTORY_RANGE-2][i]>oldSpectrumData[BEAT_FFT_HISTORY_RANGE-1][i])) beatDetected[i]=1;
            else beatDetected[i]=0;
        } else beatDetected[i]=0;
	}

	
}

- (void)windowSignalSegment {
	for (int i=0; i<self.dataLength; i++) {
		self.inputData[i] *= self.win[i];
	}
}

- (void)initWindowType {
	
	float N = self.dataLength;
	float windowval, tmp;
	float pi = 3.14159265359f;
	
	//NSLog(@"Set up all values, about to init window type %d", inWindowType);
		
	// Source: http://en.wikipedia.org/wiki/Window_function
	
	for (int i=0; i < self.dataLength; i++) {
		switch (self.windowType) {
			case kWindow_Rectangular:
				windowval = 1.0f;
				break;
			case kWindow_Hamming:
				windowval = 0.54f - 0.46f*cos((2*pi*i)/(N - 1));
				break;
			case kWindow_Hann:
				windowval = 0.5f*(1 - cos((2*pi*i)/(N - 1)));
				break;
			case kWindow_Bartlett:
				windowval  = (N-1)/2;
				tmp = i - ((N-1)/2);
				if (tmp < 0.0f) {tmp = -tmp;} 
				windowval -= tmp;
				windowval *= (2/(N-1));
				break;
			case kWindow_Triangular:
				tmp = i - ((N-1.0f)/2.0f);
				if (tmp < 0.0f) {tmp = -tmp;} 
				windowval = (2/N)*((N/2) - tmp);
				break;
		}
		self.win[i] = windowval;
	}	

}


- (BOOL)checkDataLength:(int)inDataLength {
	// Check that inDataLength is a power of two.
	// Thanks StackOverflow! (Q 600293, answered by Greg Hewgill)
	BOOL isPowerOfTwo = (inDataLength & (inDataLength - 1)) == 0;	
	// and that it's not too long. INT OVERFLOW BAD.
	BOOL isWithinIntRange = inDataLength < MAX_INT;
	
	NSAssert(isPowerOfTwo, @"The length of provided data must be a power of two");
// 	NSAssert(inDataLength <= MAX_INT, @"At this juncture, can't do an FFT on a signal longer than the maximum value for int = 32767");
	
	//NSLog(@"Everything checks out for %d samples of data", inDataLength);
	return isPowerOfTwo & isWithinIntRange;
}





#pragma mark - The End
- (void)dealloc {
	free(self.win);
	free(self.spectrumData);
	free(self.inputData);
	free(fft_w);
	free(fft_ip);
	for (int i=0;i<BEAT_FFT_HISTORY_RANGE;i++) {
		free(self.oldSpectrumData[i]);
	}
	free(self.oldSpectrumData);
	free(self.beatDetected);	
	
	[super dealloc];
	
}


@end
