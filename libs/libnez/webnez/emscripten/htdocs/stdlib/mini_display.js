/*
* Simple visualization used by my old chiptune player example pages.
*
*   version 1.02
*
* Page provides a collapsible "what's this" info box (see <details>).  For 
* background visuals an animated "frequency spectrum" is drawn as well
* as some text info describing the played song.
*
* external dependencies:
*
*	ScriptNodePlayer
*   document canvas elements: "spectrumCanvas", "logoCanvas", 
*   document div elements: "moz-reflect-spectrum", "moz-reflect-logo"
*   respective styles defined in common.css
*/


/**
* Abstract interface used to provide the data to display.
*/
class DisplayAccessor {	
	constructor(doGetSongInfo)
	{
		this.getSongInfo = doGetSongInfo;	// util for use in subclasses
	}
	
	getDisplayTitle()		{}
	getDisplaySubtitle()	{}
	getDisplayLine1()		{}
	getDisplayLine2()		{}
	getDisplayLine3()		{}
};

/**
* This renders a "frequency spectrum" based graph and some song meta information.
*/
class SongDisplay {
	/*
	* @param displayAccessor concrete subclass of DisplayAccessor
	* @param colors either the URL to an image or an array containing color strings
	* @param barType 0: "growing bars", 1: "jumping bars"
	* @param cpuLimit percentage of CPU that can be used for graphics: 0= 0%, 1=100%.. or anything inbetween
	*/
	constructor(displayAccessor, colors, barType, cpuLimit)
	{
		this._displayAccessor = displayAccessor;
		this._hexChars = "0123456789ABCDEF";
		
		if (typeof colors == 'string' || colors instanceof String) 
		{
			this._colors;
			this._backgroundImgUrl = colors;
			this._backgroundImg = 0;
		} 
		else if( Object.prototype.toString.call( colors ) === '[object Array]' ) 
		{			
			this._colors = colors;	// array containing colors
		} 
		else 
		{
			console.log("fatal error: invalid 'colors' argument");
		}
		
		this._width = 800;
		this._height = 100;
				
		this._barType = barType;
		this._barWidth = 5;
		this._barHeight = 30;
		this._barSpacing = 10;
		this._colorOffset = 0;
		
		this._refreshCounter = 0;
		this._lastRenderTime = 0;
		this._worstRenderTime = 0;		
		this._timeLimit = 1000 / 60 * cpuLimit;	// renderiung should take no longer than 10% of the available time (e.g. 3-4 millis per frame)
				
		this._canvasSpectrum = document.getElementById('spectrumCanvas');
		if (this._canvasSpectrum)
		{
			this._ctxSpectrum = this._canvasSpectrum.getContext('2d');
			this._mozReflectSpectrum = document.getElementById('moz-reflect-spectrum');
		}		
		
		this._canvasLegend = document.getElementById('logoCanvas');
		this._ctxLegend = this._canvasLegend.getContext('2d');	
		this._mozReflectLogo = document.getElementById('moz-reflect-logo');
	}
		
	_updateImage(src) 
	{
		this._backgroundImg = 0;
		let imgObj = new Image();

		imgObj.onload = function () { 
			this._backgroundImg = imgObj;			
		}.bind(this);
		
		imgObj.src = src;
	}

	setBarDimensions(w, h, s) 
	{
		this._barWidth = w;
		this._barHeight = h;
		this._barSpacing = s;
	}
	
	_drawGrowingBars(freqByteData, numBars)
	{
		const offset = 100;
		
		for (let i = 0; i < numBars; ++i) {
			this._ctxSpectrum.fillStyle = this._colorGradient(this._colors, i / numBars);
			let magnitude = freqByteData[i + offset] * this._height / 255;
			this._ctxSpectrum.fillRect(i * this._barSpacing, this._height, this._barWidth, -magnitude);
			
			this._ctxSpectrum.fillStyle= "#b75b4e";
			let d = this._caps[i] * this._decayRate;
			this._caps[i]= Math.max(d, magnitude);
			
			this._ctxSpectrum.fillRect(i * this._barSpacing, this._height - this._caps[i], this._barWidth, 3);				
		}		
	}
	
	_drawJumpingBars(freqByteData, numBars)
	{
		const offset = 100;
		
		for (let i = 0; i < numBars; ++i) {
			let scale = freqByteData[i + offset] / 0xff;
			let magnitude = scale * this._height;

			let p = Math.abs((i + this._colorOffset) % numBars);
			
			this._ctxSpectrum.fillStyle = this._colorGradient(this._colors, p / numBars);	// computationally much less expensive than previous impl with use of a scaled image
			this._ctxSpectrum.fillRect(i * this._barSpacing, this._height - magnitude, this._barWidth, this._barHeight * (scale * 1.4));
		}
	}

	_drawJumpingImageBars(freqByteData, numBars)
	{
		const offset = 100;
				
		for (let i = 0; i < numBars; ++i) {
			let scale = freqByteData[i + offset] / 0xff;
			let magnitude = scale * this._height;
			let o = Math.round(this._height - magnitude);
			this._ctxSpectrum.drawImage(this._backgroundImg, 0,0, this._barWidth, 255, 
										i * this._barSpacing, o, this._barWidth, 30 * (scale * 1.4));
		}
	}
	
	_drawGrowingImageBars(freqByteData, numBars)
	{
		const offset = 100;
		
		let imgW = this._backgroundImg.naturalWidth;			
		let imgH = this._backgroundImg.naturalHeight;			
		let sliceW = imgW / numBars;
		
		for (let i = 0; i < numBars; ++i) {
			let magnitude = freqByteData[i + offset] * this._height / 255;
			let o = Math.round(this._height - magnitude);
			this._ctxSpectrum.drawImage(this._backgroundImg, i * sliceW, 0, sliceW, imgH, 
										i * this._barSpacing, o, this._barWidth, Math.round(magnitude));
		}					
	}
	
	_throttledCall(func)
	{
		// with low enough load the browser should be able to render 60fps: the load from the music generation
		// cannot be changed but the load from the rendering can..
		this._worstRenderTime = Math.max(this._worstRenderTime, this._lastRenderTime);

		this._lastRenderTime = new Date().getTime();	// start new measurement

		let slowdownFactor = this._timeLimit ? Math.max(1, this._worstRenderTime / this._timeLimit) : 1;
		this._refreshCounter++;

		if (this._refreshCounter >= slowdownFactor) 
		{
			this._refreshCounter = 0;
						
			func();
		}		
		this._lastRenderTime= new Date().getTime()-this._lastRenderTime;
	}
	
	_redrawSpectrum() 
	{
		if (this._canvasSpectrum)
		{
			let freqByteData = ScriptNodePlayer.getInstance().getFreqByteData();
			
			let numBars = Math.round(this._width / this._barSpacing);

			if(typeof this._caps === 'undefined') 
			{
				this._caps = new Array(numBars);
				this._decayRate = 0.99;
				for (let i = 0; i < numBars; i++) this._caps[i] = 0;
			}
			
			try {
				// seems that dumbshit Safari (11.0.1 OS X) uses the fillStyle for "clearRect"!
				this._ctxSpectrum.fillStyle = "rgba(0, 0, 0, 0.0)";
			} catch (err) {}

			this._ctxSpectrum.clearRect(0, 0, this._width, this._height);

			this._ctxSpectrum.lineCap = 'round';
			
			if (this._colors) // colors based effects
			{
				if (this._barType & 0x2) 
				{
					this._colorOffset -= 2;	// cycle colors
				}
				
				if ((this._barType & 0x1) == 1) 
				{
					this._drawJumpingBars(freqByteData, numBars);
				} 
				else 
				{
					this._drawGrowingBars(freqByteData, numBars);
				}
			} 
			else // background image based effects
			{				
				if (this._backgroundImg) 
				{
					if (this._barType == 1) 
					{
						this._drawJumpingImageBars(freqByteData, numBars);
					} 
					else 
					{
						this._drawGrowingImageBars(freqByteData, numBars);
					}
				}
				
			}
			// hack: make sure dumb Firefox knows that redraw is needed..
			this._mozReflectSpectrum.style.visibility = "hidden";
			this._mozReflectSpectrum.style.visibility = "visible";
		}
	}
	
	redrawSpectrum() 
	{
		this._throttledCall(this._redrawSpectrum.bind(this));
	}
	
	_text(ctx, t, x, y) 
	{
		if(typeof t === 'undefined') return;
				
		t = t.replace(/&#.*?;/g, function (needle) {	// replace html chars "&#111;"
			return String.fromCharCode( parseInt(needle.substring(2, needle.length-1)));
		}.bind(this));

		ctx.strokeText(t, x, y);
		ctx.fillText(t, x, y);
	}
	
	redrawSongInfo() 
	{
		if (!this._colors && !this._backgroundImg) this._updateImage(this._backgroundImgUrl);	
		
		try {
			// seems that the Safari (11.0.1 OS X) idiots use the fillStyle for 
			// "clearRect", i.e. it is impossible to properly make this canvas 
			// completely transparent
			this._ctxLegend.fillStyle = "rgba(0, 0, 0, 0.0)";
		} catch (err) {}
		
		this._ctxLegend.clearRect(0, 0, 800, 300);
		
		this._ctxLegend.textBaseline = "middle";
		this._ctxLegend.fillStyle = '#000';
		this._ctxLegend.strokeStyle = "#FFFFFF";
		
		this._ctxLegend.font = '30px arial, sans-serif';	// Chrome garbage no longer understands: '90px serif bold'
		
		this._text(this._ctxLegend, this._displayAccessor.getDisplayTitle(), 20, 15);
		
		this._ctxLegend.font = '15px sans-serif';
		this._text(this._ctxLegend, this._displayAccessor.getDisplaySubtitle(), 20, 35);

		this._ctxLegend.fillStyle = '#888';
		this._ctxLegend.font = '15px sans-serif';

		this._ctxLegend.textBaseline = 'bottom';

		this._text(this._ctxLegend, this._displayAccessor.getDisplayLine1(), 20, 65);
		this._text(this._ctxLegend, this._displayAccessor.getDisplayLine2(), 20, 80);
		this._text(this._ctxLegend, this._displayAccessor.getDisplayLine3(), 20, 95);	
	}
	
	_colorGradient(cols, s) 
	{
		let p = (cols.length - 1) * s;
		let i = Math.floor(p);
		
		return this._fadeColor(cols[i], cols[i + 1], p - i);
	}
	
	_fadeColor(from, to, s) 
	{
		let r1 = (from >> 16) & 0xff;
		let g1 = (from >> 8) & 0xff;
		let b1 = from & 0xff;

		let r = Math.round(r1 + (((to >> 16) & 0xff) - r1) * s);
		let g = Math.round(g1 + (((to >> 8) & 0xff) - g1) * s);
		let b = Math.round(b1 + ((to & 0xff) - b1) * s);
	
		return "#" + this._hex(r>>4) + this._hex(r) + this._hex(g>>4) + this._hex(g) + this._hex(b>>4) + this._hex(b);
	}
	
	_hex(n) 
	{
		return this._hexChars.charAt(n & 0xf);
	}
};


window.console || (window.console = { 'log': alert });


/**
* Initialize UI
*/
$(function() {
	
	// setup initial state of the <details> info box
	$('html').addClass($.fn.details.support ? 'details' : 'no-details');
	$('details').details();

	if (!(window.openDetails === 'undefined') && window.openDetails) {
		if ($.fn.details.support) {
			$('details').attr('open', '');	// Chrome
		} else {
			let $details= $('details');
			let $detailsSummary = $('summary', $details).first();
			let $detailsNotSummary = $details.children(':not(summary)');
			
			$details.addClass('open').prop('open', true).triggerHandler('open.details');
			$detailsNotSummary.show();
		}
	}	
	
	// hack to get rid of Firefox specific pseudo elements used to sim webkit-box-reflect
	if (!window.chrome) {
		let e = document.getElementById("moz-reflect-logo");		
		e.className += " enableMozReflection";
		let f = document.getElementById("moz-reflect-spectrum");		
		f.className += " enableMozReflection";
	}	
});	


