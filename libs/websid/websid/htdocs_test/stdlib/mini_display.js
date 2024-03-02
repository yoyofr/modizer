/**
* Simple music visualization for ScriptNodePlayer.
*
* <p>Use SongDisplay to render animated frequency spectrum and basic meta info about the current song.
*
* <p>This file also handles HTML5 <details> and their simulation in Firefox: set "window.openDetails" 
* (before loading this file) to control if "details" are initially open/closed 
*/


/**
* Accessor must be subclassed to define attribute access for a specific backend.
*/
DisplayAccessor = function (doGetSongInfo) {
	this.doGetSongInfo= doGetSongInfo;
};

DisplayAccessor.prototype = {
	getDisplayTitle: function() {},
	getDisplaySubtitle: function() {},
	getDisplayLine1: function() {},
	getDisplayLine2: function() {},
	getDisplayLine3: function() {},

	// ---------------- utilities -----------------
	getSongInfo: function() {
		return this.doGetSongInfo();
	},
};

/*
* This render class drives the "requestAnimationFrame" cycle.
*
* external dependencies:
*	 ScriptNodePlayer.getInstance()
*    document canvas elements: "spectrumCanvas", "logoCanvas", 
*    document div elements: "moz-reflect-spectrum", "moz-reflect-logo"
*
* @param displayAccessor subclass of DisplayAccessor
* @param colors either the URL to an image or an array containing color strings
* @param barType 0: "growing bars", 1: "jumping bars"
* @param cpuLimit percentage of CPU that can be used for graphics: 0= 0%, 1=100%.. or anything inbetween
* @param doAnimate some function that will be added to the built-in rendering
*/
SongDisplay = function(displayAccessor, colors, barType, cpuLimit, doAnimate) {
	this.displayAccessor= displayAccessor;
	this.hexChars= "0123456789ABCDEF";
	
	if (typeof colors == 'string' || colors instanceof String) {
		// URL of image to be used
		this.colors;
		this.backgroundImgUrl= colors;
		this.backgroundImg= 0;
	} else if( Object.prototype.toString.call( colors ) === '[object Array]' ) {
		// array containing colors
		this.colors= colors;
	} else {
		console.log("fatal error: invalid 'colors' argument");
	}
	
	this.doAnimate= doAnimate;
	this.barType= barType;
	
	this.WIDTH= 800;
	this.HEIGHT= 100;
	
	this.barWidth = 5;
	this.barHeigth= 30;
	this.barSpacing= 10;
	this.colorOffset=0;
	
	this.refreshCounter=0;
	this.lastRenderTime=0;
	this.worstRenderTime=0;
	
	this.timeLimit= 1000/60*cpuLimit;	// renderiung should take no longer than 10% of the available time (e.g. 3-4 millis per frame)
	
	this.canvasSpectrum = document.getElementById('spectrumCanvas');
	this.ctxSpectrum = this.canvasSpectrum.getContext('2d');
//	this.canvasSpectrum.width = this.WIDTH;

	this.mozReflectSpectrum = document.getElementById('moz-reflect-spectrum');
	this.mozReflectLogo = document.getElementById('moz-reflect-logo');
	
	this.canvasLegend = document.getElementById('logoCanvas');
	this.ctxLegend = this.canvasLegend.getContext('2d');	
};

SongDisplay.prototype = {
	reqAnimationFrame: function() {
		window.requestAnimationFrame(this.redraw.bind(this));
	},
	updateImage: function(src) {
		this.backgroundImg= 0;
		var imgObj = new Image();

		imgObj.onload = function () { 
			this.backgroundImg= imgObj;			
		}.bind(this);
		imgObj.src=src;
	},
	redraw: function() {
		if (!this.colors && !this.backgroundImg) this.updateImage(this.backgroundImgUrl);	
		
		if(this.doAnimate) this.doAnimate();
		this.redrawSpectrum();
		
		this.reqAnimationFrame();	
	},
	setBarDimensions: function(w, h, s) {
		this.barWidth = w;
		this.barHeigth= h;
		this.barSpacing= s;
	},
	redrawSpectrum: function() {
		// with low enough load the browser should be able to render 60fps: the load from the music generation
		// cannot be changed but the load from the rendering can..
		this.worstRenderTime= Math.max(this.worstRenderTime, this.lastRenderTime);

		this.lastRenderTime= new Date().getTime();	// start new measurement

		var slowdownFactor= this.timeLimit?Math.max(1, this.worstRenderTime/this.timeLimit):1;
		this.refreshCounter++;

		if (this.refreshCounter >= slowdownFactor) {
			this.refreshCounter= 0;
			
			var freqByteData= ScriptNodePlayer.getInstance().getFreqByteData();

			var OFFSET = 100;
			
			var numBars = Math.round(this.WIDTH / this.barSpacing);

			if(typeof this.caps === 'undefined') {
				this.caps= new Array(numBars);
				this.decayRate= 0.99;
				for (var i= 0; i<numBars; i++) this.caps[i]= 0;
			}
			
			try {
				// seems that dumbshit Safari (11.0.1 OS X) uses the fillStyle for "clearRect"!
				this.ctxSpectrum.fillStyle = "rgba(0, 0, 0, 0.0)";
			} catch (err) {}

			this.ctxSpectrum.clearRect(0, 0, this.WIDTH, this.HEIGHT);

			this.ctxSpectrum.lineCap = 'round';
			
			if (this.colors) {
				if (this.barType & 0x2) {
					this.colorOffset-= 2;	// cycle colors
					/*
					if (window.chrome) {		// FF "blur" is too slow for this
						this.ctxSpectrum.shadowBlur = 20; 
						this.ctxSpectrum.shadowColor = "black";
					}
					*/
				}
				// colors based effects
				if ((this.barType & 0x1) == 1) {
					// "jumping" bars
					 
					for (var i = 0; i < numBars; ++i) {
						var scale= freqByteData[i + OFFSET]/0xff;
						var magnitude = scale*this.HEIGHT;

						var p= Math.abs((i+this.colorOffset)%numBars);
						
						this.ctxSpectrum.fillStyle = this.colorGradient(this.colors,p/numBars);	// computationally much less expensive than previous impl with use of a scaled image
						this.ctxSpectrum.fillRect(i * this.barSpacing, this.HEIGHT- magnitude, this.barWidth, this.barHeigth*(scale*1.4));
					}
				} else {
					// "growing" bars
					for (var i = 0; i < numBars; ++i) {
						this.ctxSpectrum.fillStyle = this.colorGradient(this.colors,i/numBars);
						var magnitude = freqByteData[i + OFFSET]*this.HEIGHT/255;
						this.ctxSpectrum.fillRect(i * this.barSpacing, this.HEIGHT, this.barWidth, -magnitude);
						
						this.ctxSpectrum.fillStyle= "#b75b4e";
						var d= this.caps[i]*this.decayRate;
						this.caps[i]= Math.max(d, magnitude);
						
						this.ctxSpectrum.fillRect(i * this.barSpacing, this.HEIGHT-this.caps[i], this.barWidth, 3);				
					}		
				}
			} else {
				// background image based effects
				if (this.backgroundImg) {
					if (this.barType == 1) {
						var o;
						for (var i = 0; i < numBars; ++i) {
							var scale= freqByteData[i + OFFSET]/0xff;
							var magnitude = scale*this.HEIGHT;
							o= Math.round(this.HEIGHT - magnitude);
							this.ctxSpectrum.drawImage(this.backgroundImg, 0,0, this.barWidth, 255, 
														i * this.barSpacing, o, this.barWidth, 30*(scale*1.4));
						}
					} else {
						var imgW= this.backgroundImg.naturalWidth;			
						var imgH= this.backgroundImg.naturalHeight;			

						var sliceW= imgW/numBars;
						var o;
						for (var i = 0; i < numBars; ++i) {
							var magnitude = freqByteData[i + OFFSET]*this.HEIGHT/255;
							o= Math.round(this.HEIGHT - magnitude);
							this.ctxSpectrum.drawImage(this.backgroundImg, i*sliceW, 0, sliceW, imgH, 
														i * this.barSpacing, o, this.barWidth, Math.round(magnitude));
						}					
					}
				}
				
			}
			// hack: make sure dumb Firefox knows that redraw is needed..
			this.mozReflectSpectrum.style.visibility = "hidden";
			this.mozReflectSpectrum.style.visibility = "visible";
		}		
		this.lastRenderTime= new Date().getTime()-this.lastRenderTime;
	},
	text: function(ctx, t, x, y) {
		if(typeof t === 'undefined')
			return;
				
		t = t.replace(/&#.*?;/g, function (needle) {	// replace html chars "&#111;"
			return String.fromCharCode( parseInt(needle.substring(2, needle.length-1)));
		}.bind(this));

		
		ctx.strokeText(t, x, y);
		ctx.fillText(t, x, y);
	},
	redrawSongInfo: function() {
		this.reqAnimationFrame();	// start the animation going

		try {
			// seems that the Safari (11.0.1 OS X) idiots use the fillStyle for 
			// "clearRect", i.e. it is impossible to properly make this canvas 
			// completely transparent
			this.ctxLegend.fillStyle = "rgba(0, 0, 0, 0.0)";
		} catch (err) {}
		
		this.ctxLegend.clearRect(0, 0, 800, 300);
		//this.canvasLegend.width  += 0;
		
		this.ctxLegend.textBaseline = "middle";
		this.ctxLegend.fillStyle = '#000';
		this.ctxLegend.strokeStyle = "#FFFFFF";
		
		this.ctxLegend.font = '30px  arial, sans-serif';	// FUCKING CHROME SHIT NO LONGER UNDERSTANDS: '90px serif bold'
		
		this.text(this.ctxLegend, this.displayAccessor.getDisplayTitle(), 20, 15);
		
		this.ctxLegend.font = '15px sans-serif';
		this.text(this.ctxLegend, this.displayAccessor.getDisplaySubtitle(), 20, 35);

		this.ctxLegend.fillStyle = '#888';
		this.ctxLegend.font = '15px sans-serif';

		this.ctxLegend.textBaseline = 'bottom';

		this.text(this.ctxLegend, this.displayAccessor.getDisplayLine1(), 20, 65);
		this.text(this.ctxLegend, this.displayAccessor.getDisplayLine2(), 20, 80);
		this.text(this.ctxLegend, this.displayAccessor.getDisplayLine3(), 20, 95);
		
		// hack: make sure dumb Firefox knows that redraw is needed..
		this.mozReflectLogo.style.visibility = "hidden";
		this.mozReflectLogo.style.visibility = "visible";
	},
	colorGradient: function(cols, s) {
		var p= (cols.length-1)*s;
		var i= Math.floor(p);
		
		return this.fadeColor(cols[i], cols[i+1], p-i);
	},
	fadeColor: function(from, to, s) {
		var r1= (from >>16) & 0xff;
		var g1= (from >>8) & 0xff;
		var b1= from & 0xff;

		var r= Math.round(r1+(((to >>16) & 0xff)-r1)*s);
		var g= Math.round(g1+(((to >>8) & 0xff)-g1)*s);
		var b= Math.round(b1+((to & 0xff)-b1)*s);
	
		return "#" +this.hex(r>>4) +this.hex(r) +this.hex(g>>4) +this.hex(g) +this.hex(b>>4) +this.hex(b);
	},
	hex: function(n) {
		return this.hexChars.charAt(n & 0xf);
	}
};

window.console || (window.console = { 'log': alert });
$(function() {
	$('html').addClass($.fn.details.support ? 'details' : 'no-details');
	$('details').details();

	// initially expand details
	if (!(window.openDetails === 'undefined') && window.openDetails) {
		if ($.fn.details.support) {
			$('details').attr('open', '');	// Chrome
		} else {
			var $details= $('details');
			var $detailsSummary = $('summary', $details).first();
			var $detailsNotSummary = $details.children(':not(summary)');
			
			$details.addClass('open').prop('open', true).triggerHandler('open.details');
			$detailsNotSummary.show();
		}
	}
	if (!window.chrome) {
		// hack to get rid of Firefox specific pseudo elements used to sim webkit-box-reflect
		var e = document.getElementById("moz-reflect-logo");		
		e.className += " enableMozReflection";
		var e2 = document.getElementById("moz-reflect-spectrum");		
		e2.className += " enableMozReflection";
	}
});	


