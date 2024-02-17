/**
* Simple "ticker" plugin that serves as an example for how the AbstractTicker API
* can be used to extend the scriptprocessor_player.js (see ScriptNodePlayer.createInstance) 
*
* It is designed for use with the backend_tinyrsid.js and provides the "realtime" raw data of the 
* underlying SID voices corresponding to the music played via WebAudio.
*
* Underlying/addressed problems: 
*
* 1) The scriptprocessor_player.js uses WebAudio with an audio buffer size of 8192 samples. This
* relatively large buffer reduces the likelyhood that music playback runs out of data (and suddenly 
* starts stuttering). At 44100Hz that amount of data will last for 0.186 secs and WebAudio will request 
* a new buffer some time *before* it actually needs it. This makes it rather hard to make data usage 
* timings that require a higher precision (e.g. to display in "realtime" what is being played back).
*
* 2) The underlying emulator creating the "audio samples" does so with a specific playback frequency in 
* mind (e.g. 48000Hz) and there is a specific batch size that it produces with every call (e.g. 882 
* samples). WebAudio may be running at 41100Hz instead and when it asks for another chunk of 8192 samples
* these are served by calling the emulator, resampling its output to the WebAudio playback frequency and 
* repeating that process until enough data has been collected in WebAudio's audio buffer.
*
* Solution concept:
* 
* The infrastructure needed to deal with the above is implemented within scriptprocessor_player.js: 
*
* 1) An additional "timing node" is introduced into the WebAudio configuration. This node runs with the 
* same frequency and in-sync with the node that actually produces the music. But contrary to the "music 
* node" this "timing node" uses a small 256 samples buffer. This means that it will run out of data 32 times
* more quickly (its data here only lasts for 0.0058 secs). Its requests for new data are used to calculate
* how far WebAudio has advanced in its audio buffer playback. 
*
* 2) The emulator generates add-on data with the exact same granulatory as the audio samples, i.e. if there is
* one audio sample then there is exactly on element of some type of add-on data (respective output arrays 
* have the same size). Whatever transformations are later done with the audio buffer (see 2. above) are also 
* performed with the add-on data. Since the add-on data is backend specific it cannot be done directly 
* within the scriptprocessor_player.js but instead scriptprocessor_player.js delegates the backend specific 
* stuff to a backend specific subclass of AbstractTicker. The below SidTracer is such a subclass.
*
* Functionality:
*
* This implementation provides a view into the past. Its getters do NOT provide data that will be played in 
* the future (eventhough *some* of it may already be available in the buffers..). The respective view 
* may reach back into the past for up to 8192 samples and includes the sample data that is played just now (within 
* the "256 samples" precision limitation).
*
*
* version 1.0
*
* 	Copyright (C) 2018 Juergen Wothke
*
* Terms of Use: This file is free software. (Other licenses may apply to the remaining 
* files used in this example.)
*/
SidTracer = (function(){ var $this = function(outputSize) { 
		$this.base.call(this);
		
		this.outputSize= outputSize;
	}; 
	extend(AbstractTicker, $this, {
		
	// ------------ APIs expected by the base infractructure - see doc in AbstractTicker parent class -----------------
		
		/*
		* Basic initialization: 4 kinds of add-on data (voice 1-4) are use here, and for each of these
		*                       3  buffers are used during precessing (see "step" comments below)
		*/
		init: function(samplesPerBuffer, tickerStepWidth) {
			this._tickerStepWidth= tickerStepWidth;
			this._samplesPerBuffer= samplesPerBuffer;
			this._backendAdapter; 	// initialized later

			// 1st step: make sure same resampling is used for "add-on data" (as is used for the audio data)
			this._resampleV1= new Float32Array(samplesPerBuffer);
			this._resampleV2= new Float32Array(samplesPerBuffer);
			this._resampleV3= new Float32Array(samplesPerBuffer);
			this._resampleV4= new Float32Array(samplesPerBuffer);	// the "digi channel"

			
			// 2nd step: "add-on" buffers in sync with the WebAudio audio buffer.
			// 			 use double buffering to allow smooth transitions at buffer boundaries
			//           (2 buffers are sufficient for the sliding window impl of a "past & present" view )
			this._activeBufIdx = 0;	
			this._buffers= [
								[ new Float32Array(samplesPerBuffer),		// voice 1 
									new Float32Array(samplesPerBuffer), 	// voice 2
									new Float32Array(samplesPerBuffer), 	// voice 3
									new Float32Array(samplesPerBuffer) ],	// digi
									
								[ new Float32Array(samplesPerBuffer),		// voice 1 
									new Float32Array(samplesPerBuffer), 	// voice 2
									new Float32Array(samplesPerBuffer), 	// voice 3
									new Float32Array(samplesPerBuffer) ]	// digi
							];
			
			// 3rd step: use "sliding window" approach to fill the actual output buffers
			this.setOutputSize(this.outputSize);					
		},	
		/**
		* Marks the start of a new audio buffer, i.e. a new audio buffer is about to be generated.
		*/
		start: function() {
			// alternate used output buffers (as base for "sliding window" data access) 			
			this._activeBufIdx = (this._activeBufIdx ? 0 : 1);	
		},
		/**
		* Corresponds to one emulator call - after which resampling of its output is required.
		*/
		resampleData: function(sampleRate, inputSampleRate, inputLen, backendAdapter) {
						
			this._backendAdapter= backendAdapter; // not the best place for this ...
			
			// this makes sure that the raw data is still in sync after the resampling of the audio data (emulator output)			
			var resampleLen;
			if (sampleRate == inputSampleRate) {		
				resampleLen= inputLen;
			} else {
				resampleLen= Math.round(inputLen * sampleRate / inputSampleRate);	
			}
			
			// cache resampled data in more accessible form (allocated size will quickly stabilize)
			this._resampleV1= this._assertBuffer(backendAdapter, this._resampleV1, resampleLen);
			this._resampleV2= this._assertBuffer(backendAdapter, this._resampleV2, resampleLen);
			this._resampleV3= this._assertBuffer(backendAdapter, this._resampleV3, resampleLen);
			this._resampleV4= this._assertBuffer(backendAdapter, this._resampleV4, resampleLen);
			
			// resample the add-on data if necessary
			this._resample(backendAdapter.getBufferVoice1(), inputLen, this._resampleV1, resampleLen, backendAdapter);
			this._resample(backendAdapter.getBufferVoice2(), inputLen, this._resampleV2, resampleLen, backendAdapter);
			this._resample(backendAdapter.getBufferVoice3(), inputLen, this._resampleV3, resampleLen, backendAdapter);
			this._resample(backendAdapter.getBufferVoice4(), inputLen, this._resampleV4, resampleLen, backendAdapter);
		},
		/**
		* Mirrors the transfer of the resampled audio data into WebAudio's audio buffer.
		*/
		copyTickerData: function(outBufferIdx, inBufferIdx) {
			// the buffers filled here then are in sync with the respective audio data buffer used by WebAudio 
			var buf= this._buffers[this._activeBufIdx];	

			buf[0][outBufferIdx]= this._resampleV1[inBufferIdx];
			buf[1][outBufferIdx]= this._resampleV2[inBufferIdx];
			buf[2][outBufferIdx]= this._resampleV3[inBufferIdx];
			buf[3][outBufferIdx]= this._resampleV4[inBufferIdx];
		},
		
	// ------------ API specifically provided by SidTracer (invent whatever accessors you need here) -----------------

		/*
		* Sets the size of the backward-view (in number of samples). Cannnot be larger than this._samplesPerBuffer
		*/
		setOutputSize(s) {
			if (s > this._samplesPerBuffer) {
				console.log("error: max output size is " + this._samplesPerBuffer);
				s= this._samplesPerBuffer;
			}		
			this.outputSize= s;
			
			if ((typeof this._outputV1 == 'undefined') || (this._outputV1.length != s)) {
				this._outputV1= new Float32Array(s);
				this._outputV2= new Float32Array(s);
				this._outputV3= new Float32Array(s);
				this._outputV4= new Float32Array(s);	// the "digi channel"
			}
		},
				
		/*
		* NOTE: due to the fact that the below functions return a fixed size "sliding window" of 
		* data, the data returned by successive calls may overlap - or be disjoint when calls are 
		* spaced too far apart.. 
		*
		* In order to let the caller collect/append data (e.g. to display some zoomed-out
		* range) it might be useful to have other APIs, e.g. that deliver only new data or
		* give some feedback with "tick" related info - so that the caller may perform the
		* respective calculations himself, etc. But for now I leave that as an exercise to the reader.
		*/
		getDataVoice1: function() {
			this._copySlidingWindow(0, this._outputV1);
			return this._outputV1;
		},
		getDataVoice2: function() {
			this._copySlidingWindow(1, this._outputV2);
			return this._outputV2;
		},
		getDataVoice3: function() {
			this._copySlidingWindow(2, this._outputV3);
			return this._outputV3;
		},
		getDataVoice4: function() {
			this._copySlidingWindow(3, this._outputV4);
			return this._outputV4;
		},	
		getRegisterSID: function(reg) {
			return (typeof(this._backendAdapter) == 'undefined') ? 'undefined' : this._backendAdapter.getRegisterSID(reg);
		},
		getDigiType: function(reg) {
			return (typeof(this._backendAdapter) == 'undefined') ? 'undefined' : this._backendAdapter.getDigiType();
		},
		getDigiTypeDesc: function(reg) {
			return (typeof(this._backendAdapter) == 'undefined') ? 'undefined' : this._backendAdapter.getDigiTypeDesc();
		},
		getDigiRate: function(reg) {
			return (typeof(this._backendAdapter) == 'undefined') ? 'undefined' : this._backendAdapter.getDigiRate();
		},
	
	
	// ------------------------- private utility functions ---------------------------
		
		_resample: function(inputPtr, inputLen, resampledBuffer, resampleLen, backendAdapter) {			
			if (resampleLen == inputLen) {
				backendAdapter.getCopiedAudio(inputPtr, inputLen, backendAdapter.readFloatSample.bind(backendAdapter), resampledBuffer);
			} else {
				backendAdapter.resampleToFloat(1, 0, inputPtr, inputLen, backendAdapter.readFloatSample.bind(backendAdapter), resampledBuffer, resampleLen);
			}
		},		
		_assertBuffer: function(backendAdapter, buf, size) {
			if (size > buf.length) { buf= backendAdapter.allocResampleBuffer(size); }
			return buf;
		},
		_copySlidingWindow: function(voiceId, destBuffer) {
			var bufs= this._buffers[this._activeBufIdx];	
			var prevBufs= this._buffers[this._activeBufIdx ? 0 : 1];	// previous buffers
			
			var inputBuf= bufs[voiceId];
			var prevInputBuf= prevBufs[voiceId];
			
			var outputLen= destBuffer.length;
			
			// this is the crucial bit that tells where in the data is *NOW* (below is just an example
			// of how that information might be used)
			var endOffset= (ScriptNodePlayer.getInstance().getCurrentTick()+1)*this._tickerStepWidth;

			if (endOffset > outputLen) {
				// no buffer boundary crossed (simple copy from the current buffer does the job)
				this._arrayCopy(inputBuf, endOffset-outputLen, outputLen, destBuffer, 0);
			} else {
				// some data still must be fetched from the previous buffer
				var sizePrevious= outputLen - endOffset;
				this._arrayCopy(prevInputBuf, prevInputBuf.length-sizePrevious, sizePrevious, destBuffer, 0);
				
				this._arrayCopy(inputBuf, 0, endOffset, destBuffer, sizePrevious);
			}
		},
		// caution: does not check boundary violations - you better know what you are doing
		_arrayCopy(src, srcOffset, len, dest, destOffset) {
			// riddiculous that this kind of function does not come with this joke of a 
			// language (and I am not talking about the expensive reallocation workarounds..)
			
			for (var i=0; i<len; i++) {
				dest[destOffset+i]= src[srcOffset+i];
			}
		}		
});	return $this; })();	


/*
* Example for how to access SID register data.
*/
RegDisplay = function(spanId, tracer) {
		this.span = document.getElementById(spanId);
		this.sidTracer= tracer;
};

RegDisplay.prototype = {
	reqAnimationFrame: function() {
		window.requestAnimationFrame(this.redraw.bind(this));
	},
	redrawText: function() {
		this.span.innerHTML = (!this.sidTracer.getDigiType() ? "" :
								"'"+this.sidTracer.getDigiTypeDesc() +"' digi samples rate " +this.sidTracer.getDigiRate() +" Hz; "
								)+
								"d400: "+this.sidTracer.getRegisterSID(0).toString(16)+
								" d401: "+this.sidTracer.getRegisterSID(1).toString(16)+ 
								"... d404: "+this.sidTracer.getRegisterSID(4).toString(16)+" ..";
	},
	redraw: function() {
		this.redrawText();
		this.reqAnimationFrame();	
	}
};

/**
* Code losely based on C# implementation of SidWiz2 by RushJet1 & Rolf R Bakke (see respctice Form1.cs).
*
* note: original implementation included logic for multi-voice/-column layout whereas this implementation 
* renders exactely one voice and always uses 1 column. Instead of rendering a 
* complete sample file this implementation only renders the frame corresponding to the currently played music.
*
* note: the maximum "scale" that can be used is limited by the size of the sample data array passed to "draw()". 
* In order to use the maximum scale make sure the player delivers 16k of sample data..
* 
* note: original implementation used 0-255 integer range sample data - while a -1 to 1 float range is used here.
*/
SidWiz = function(width, height, ctx, altsync) {
	// graphics context to draw in:
	this._resX= width;
	this._resY= height;
	this.ctx= ctx;
	
	this._voiceData= null;	// new data is fed in for each frame
	
	this._altSync= altsync;	// enable "alt sync" impl for the voice

	this._scales = [0.125, 0.25, 0.5, 1, 2, 4,   8,   16  ];
	this._centers= [32,    16,   8,   4, 2, 1,   0.5, 0.25];		
};

SidWiz.prototype = {
	getTriggerLevel: function(jua, jac, offset) {
		// scan for peak values
		var yMax= -1;    	// was 0                                  
		var yMin= 1;		// was 255
		var juaHalf= Math.floor(jua/2);
		var s= juaHalf-jac;
		for (var h = s; h <= (juaHalf+jac); h++) {	// uses 2 center frames.. ? 
			var value = this._voiceData[offset+h];
			if (value > yMax) { yMax = value; }
			if (value < yMin) { yMin = value; }
		}
		return ((yMin + yMax) / 2);   //the middle line of the waveform
	},
	draw: function(data, scaleIdx) {
		var sampleRate= window._gPlayerAudioCtx.sampleRate;	// anyway depends on 'ticker' infrastructure from scriptprocessor_player.js (so this addition doesn't hurt)
		
		this._voiceData= data;
	
		var scale= this._scales[scaleIdx];
		var center= this._centers[scaleIdx];

		// note: the crappy variable naming of the original implementation has been
		// largely preserved to ease complarisons..
		
		var jumpAmount = (sampleRate / 60);         	// samples per frame (no need to use browser's actual framerate)		
		var jua = jumpAmount * Math.floor(1+ scale);	// jua is the size of sample data used per frame
				
		var oldY2 = 0; 
		var newY2 = 0;
		var newX = 0;
		var oldX = 0;	// was called "oldZ" in original impl (probably to reflect its use in 3-byte pixel logic)
			
		// offset to the position of the 1st sample 
		// note: the below logic expects that "jua" samples can be read starting at that position 
		var offset = (data.length - jua);

		//jac is the search window
		var jac = jumpAmount;
		
		var triggerLevel= this.getTriggerLevel(jua, jac, offset);
		
		var c= scaleIdx <=4 ? 0 : 2*jac;	// correction seems to be needed to properly position the displayed range...
		
		var frameTriggerOffset = 0;
		
		// syncronization
		if (this._altSync == false) {
			var one= 2.0 / 255;		// adjust original logic to the sample data range used here..
			var triggerLevelM= triggerLevel - one;
			var triggerLevelP= triggerLevel + one;
			
			frameTriggerOffset = jac;
			
			while (this._voiceData[offset+c+frameTriggerOffset] < (triggerLevelP) && frameTriggerOffset < jac * 2) frameTriggerOffset++;
			while (this._voiceData[offset+c+frameTriggerOffset] >= (triggerLevelM) && frameTriggerOffset < jac * 2) frameTriggerOffset++;
			if (frameTriggerOffset == jac * 2) frameTriggerOffset = 0;
			
		} else {
			var distances = [];	// array of arrays
			var qx = jac;
			while ((this._voiceData[offset+qx] >= triggerLevel) && (qx < jac * 2)) qx++;
			var ctr;
			while (qx < jac * 2) {
				ctr = qx;
				var isUp = false;
				//find point where crosses midline
				if (this._voiceData[offset+qx] < triggerLevel) {
					while ((this._voiceData[offset+qx] < triggerLevel) && (qx < jac * 2)) qx++;
					isUp = true;
				} else { 
					while ((this._voiceData[offset+qx] >= triggerLevel) && (qx < jac * 2)) qx++; 
				}

				//add point to data
				if (!isUp) {
					var data = [qx - ctr, qx];	// difference, position of the offset
					distances.push(data);
				}
			} 
			
			ctr = 0; //count of highest values
			var highest = [0, 0]; //this will be the highest value
			
			var data;
			for (data of distances) {
				if (data[0] > highest[0]) {
					highest= [data[0], data[1]];
					ctr = 1;
				} else if (data[0] == highest[0]) {
					highest.push(data[1]);
					ctr++;
				}
			}
			//at this point "highest" should be a list where the first value is the difference, and the rest are points in order where the difference occurred
			//ctr is the number of same values. if more than 95% it's probably a square wave 
			if (ctr != 1) ctr = Math.ceil(ctr / 2.0);
			frameTriggerOffset = highest[ctr];
		}
	
		// draw waveform	
		var oldY2;		// previous y coord
		for (var x = 0; (x / (scale / 2)) < this._resX; x++) {         
			var vdPos = frameTriggerOffset + c + x - Math.floor(this._resX / center); // note: stabilization causes first "c" samples to be "unusable", i.e. skip them

			if (vdPos < 0) { vdPos = 0; }
			var vdSet = this._voiceData[offset+vdPos];
			
			var y = Math.floor((vdSet+1)/2 * this._resY);	// use full available height (calc adjusted to sample range used here) 

			if (x == 0) {
				oldY2 = y;
			}
			
			newY2 = y;
			
			if (oldY2 > this._resY) oldY2 = this._resY;
			if (newY2 > this._resY) newY2 = this._resY;
			if (newY2 < 0) newY2 = 0;
			if (oldY2 < 0) oldY2 = 0;

			newX = Math.floor(2*x/scale);	// called "z" in original code
			
			if (oldY2 > newY2) { //waveform moved down
				var t = oldY2;
				oldY2 = newY2;
				newY2 = t;
			}
								
			// draw line
			if (x == 0) {
				this.ctx.moveTo(newX, this._resY-newY2);
			} else {
				this.ctx.lineTo(newX, this._resY-newY2);
			}			
			
			oldX = newX;
			oldY2 = y;
		}
	}
};


/*
* Example for basic use/rendering of streamed "add-on" data.
*/
VoiceDisplay = function(divId, getDataFunc, altsync) {
	this.WIDTH= 512;
	this.HEIGHT= 80;

	this.divId= divId;
	this.getData= getDataFunc;
	
	this.canvas = document.getElementById(this.divId);
	this.ctx = this.canvas.getContext('2d');
	this.canvas.width = this.WIDTH;
	this.canvas.height = this.HEIGHT;

	this.sidWiz= new SidWiz(this.WIDTH, this.HEIGHT, this.ctx, altsync);	
};

VoiceDisplay.prototype = {
	reqAnimationFrame: function() {
		window.requestAnimationFrame(this.redraw.bind(this));
	},
	redraw: function() {
		this.redrawGraph();		
		this.reqAnimationFrame();	
	},
	redrawGraph: function(osciloscopeMode, zoom) {
		var data= this.getData();
		
		try {
			// seems that dumbshit Safari (11.0.1 OS X) uses the fillStyle for "clearRect"!
			this.ctx.fillStyle = "rgba(0, 0, 0, 0.0)";
		} catch (err) {}
		this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
				
		this.ctx.strokeStyle = "rgba(1, 0, 0, 1.0)";
		this.ctx.save();

		if (!osciloscopeMode) {
			// zooming performed by changing abount of delivered data
			var rescale= this.WIDTH/data.length;
			this.ctx.scale(rescale, 1);	// fit all the data into the visible area
		}			
		
		this.ctx.beginPath();
				
		if (osciloscopeMode) {
			this.sidWiz.draw(data, 2+zoom);
		} else {
			for (var i = 0; i < data.length; i++) {
				var scale= (data[i]+1)/2;
				var magnitude = scale*this.HEIGHT;

				// invert Y or graphs will be upside down
				if (i == 0) {
					this.ctx.moveTo(i, this.canvas.height-magnitude);
				} else {
					this.ctx.lineTo(i, this.canvas.height-magnitude);
				}			
			}
		}
		this.ctx.stroke();		
		this.ctx.restore();
	}
};

