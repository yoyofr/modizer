/**
* Minimal controls for ScriptNodePlayer.
*
* <p>Features an initial playlist, drag&drop of additional music files, and controls for "play", "pause", 
* "next song", "previous song", "seek" (optional), "volume".
*
* <p>This crude UI is not meant to be reusable but to show how the ScriptNodePlayer is used. ()
*/
BasicPlayerControls = function(songs, enableSeek, enableSpeedTweak, doParseUrl, doOnDropFile, current, defaultOptions) {
	this._doOnDropFile= doOnDropFile;
	this._options= (typeof defaultOptions != 'undefined') ? defaultOptions : {};
	this._current= (typeof current != 'undefined')?current:-1;
	if(Object.prototype.toString.call( songs ) === '[object Array]') {
		this._someSongs=  songs;
	} else {
		console.log("warning: no valid song list supplied.. starting empty");
		this._someSongs= [];
	}
	this._enableSeek= enableSeek;
	this._enableSpeedTweak= enableSpeedTweak;
	this._doParseUrl = doParseUrl;	
	
	this.initDomElements();
};

BasicPlayerControls.prototype = {
	// facade for player functionality so that BasicPlayerControls user does not also need to know the player
	pause: function() 				{ ScriptNodePlayer.getInstance().pause(); },
	resume: function() 				{ ScriptNodePlayer.getInstance().resume(); },
	setVolume: function(value) 		{ ScriptNodePlayer.getInstance().setVolume(value); },
	getSongInfo: function ()		{ return ScriptNodePlayer.getInstance().getSongInfo(); },
	
	addSong: function(filename) {
		this._someSongs.push(filename);
	},
	seekPos: function(relPos) {
		var p= ScriptNodePlayer.getInstance();
		p.seekPlaybackPosition(Math.round(p.getMaxPlaybackPosition()*relPos));
	},	
	// some playlist handling
	removeFromPlaylist: function(songname) {
		if (this._someSongs[this._current] == songname) {
			this._someSongs.splice(this._current, 1);
			if (this._current + 1 == this._someSongs.length) this._current= 0;
		}
	},
	playNextSong: function() {
		var ready= ScriptNodePlayer.getInstance().isReady();
		if (ready && this._someSongs.length) {
			this._current= (++this._current >=this._someSongs.length) ? 0 : this._current;
			var someSong= this._someSongs[this._current];			
			this.playSong(someSong);
		}
	},
	playPreviousSong: function() {
		if (ScriptNodePlayer.getInstance().isReady() && this._someSongs.length) {	
			this._current= (--this._current<0) ? this._current+this._someSongs.length : this._current;
			var someSong= this._someSongs[this._current];
			this.playSong(someSong);
		}
	},
	
	playSongWithBackand: function (options, onSuccess) {
		// backend adapter to be used has been explicitly specified	
		var o= options.backendAdapter;
		ScriptNodePlayer.createInstance(o.adapter, o.basePath, o.preload, o.enableSpectrum, 
										onSuccess, o.doOnTrackReadyToPlay, o.doOnTrackEnd);
	},
	playSong: function(someSong) {
		var audioCtx= ScriptNodePlayer.getInstance().getAudioContext();	// handle Google's bullshit "autoplay policy"
		if (audioCtx.state == "suspended") {
			var modal = document.getElementById('autoplayConfirm');	
			modal.style.display = "block";		// force user to click
			
			window.globalDeferredPlay = function() {	// setup function to be used "onClick"
				audioCtx.resume();
				this._playSong(someSong);
			}.bind(this);
			
		} else {
			this._playSong(someSong);
		}
	},
	_playSong: function(someSong) {
		var arr= this._doParseUrl(someSong);
		var options= arr[1];
		if (typeof options.backendAdapter != 'undefined') {
			var name= arr[0];
			var o= options.backendAdapter;
			this.playSongWithBackand(options, (function(){
													var p= ScriptNodePlayer.getInstance();
													
													p.loadMusicFromURL(name, options, 
														(function(filename){
														}), 
														(function(){ 
															this.removeFromPlaylist(someSong);	/* no point trying to play this again */ }.bind(this)), 
														(function(total, loaded){}));
													
													o.doOnPlayerReady();			
												}.bind(this)));
		} else {
			var p= ScriptNodePlayer.getInstance();
			if (p.isReady()) {
				p.loadMusicFromURL(arr[0], options, 
				(function(filename){}), 
				(function(){ this.removeFromPlaylist(someSong);	/* no point trying to play this again */ }.bind(this)), 
				(function(total, loaded){}));
			}
		}
	},
	animate: function() {
		// animate playback position slider
		var slider = document.getElementById("seekPos");
		if(slider && !slider.blockUpdates) {
			var p= ScriptNodePlayer.getInstance();	
			slider.value = Math.round(255*p.getPlaybackPosition()/p.getMaxPlaybackPosition());
		}	
	},

	// ---------------------    drag&drop feature -----------------------------------
	dropFile: function(checkReady, ev, funcName, options, onCompletion) {
		ev.preventDefault();
		var data = ev.dataTransfer.getData("Text");
		var file = ev.dataTransfer.files[0];
		var p= ScriptNodePlayer.getInstance();
		
		if ((!checkReady || ScriptNodePlayer.getInstance().isReady()) && file instanceof File) {
			if (this._doOnDropFile) {
				var options= this._doOnDropFile(file.name);	// get suitable backend, etc
				var o= options.backendAdapter;

				this.pause();	// don't play while reconfiguring..

				this.playSongWithBackand(options, (function(){
													var p= ScriptNodePlayer.getInstance();
													var f= p[funcName].bind(p);

													f(file, options, 
														onCompletion, 
														(function(){ /* fail */
															this.removeFromPlaylist(file.name);	/* no point trying to play this again */ 
														}.bind(this)), 
														(function(total, loaded){})	/* progress */
													);
											
													o.doOnPlayerReady();			
												}.bind(this)));
												
			} else {
				var p= ScriptNodePlayer.getInstance();
				var f= p[funcName].bind(p);
				f(file, options, onCompletion, (function(){console.log("fatal error: tmp file could not be stored");}), (function(total, loaded){}));
			}
		}
	},
	drop: function(ev) {
		this.dropFile(true, ev, 'loadMusicFromTmpFile', this._options, (function(filename){		
						this.addSong(filename);
					}).bind(this));
	},
	
	initExtensions: function() {},	// to be overridden in subclass
	
	allowDrop: function(ev) {
		ev.preventDefault();
		ev.dataTransfer.dropEffect = 'move'; 	// needed for FF
	},
	initUserEngagement: function() {
		// handle Goggle's latest "autoplay policy" bullshit (patch the HTML/Script from here within this
		// lib so that the various existing html files need not be touched)
						
		var d = document.createElement("DIV");
		d.setAttribute("id", "autoplayConfirm");
		d.setAttribute("class", "modal-autoplay");

		var dc = document.createElement("DIV");
		dc.setAttribute("class", "modal-autoplay-content");
		
		var p = document.createElement("P");
		var t = document.createTextNode("You may thank the clueless Google Chrome idiots for this useless add-on dialog - without which their \
		user unfriendly browser will no longer play the music (which is the whole point of this page).\
		Click outside of this box to continue.");
		p.appendChild(t);
		
		dc.appendChild(p);
		d.appendChild(dc);
		
		document.body.insertBefore(d, document.body.firstChild);

		
		var s= document.createElement('script');
		s.text = 'var modal = document.getElementById("autoplayConfirm");\
			window.onclick = function(event) {\
				if (event.target == modal) {\
					modal.style.display = "none";\
					if (typeof window.globalDeferredPlay !== "undefined") { window.globalDeferredPlay();\
					delete window.globalDeferredPlay; }\
				}\
			}';
		document.body.appendChild(s);

	},
	
	
	initTooltip: function() {
		var tooltipDiv= document.getElementById("tooltip");

		var f = document.createElement("form");
		f.setAttribute('method',"post");
		f.setAttribute('action',"https://www.paypal.com/cgi-bin/webscr");
		f.setAttribute('target',"_blank");

		var i1 = document.createElement("input");
		i1.type = "hidden";
		i1.value = "_s-xclick";
		i1.name = "cmd";
		f.appendChild(i1);  
		
		var i2 = document.createElement("input");
		i2.type = "hidden";
		i2.value = "E7ACAHA7W5FYC";
		i2.name = "hosted_button_id";
		f.appendChild(i2);  
		
		var i3 = document.createElement("input");
		i3.type = "image";
		i3.src= "stdlib/btn_donate_LG.gif";
		i3.border= "0";
		i3.name="submit";
		i3.alt="PayPal - The safer, easier way to pay online!";
		f.appendChild(i3);  
		
		var i4 = document.createElement("img");
		i4.alt = "";
		i4.border = "0";
		i4.src = "stdlib/pixel.gif";
		i4.width = "1";
		i4.height = "1";
		f.appendChild(i4);  
		
		tooltipDiv.appendChild(f);  
	},
	initDrop: function() {
		// the 'window' level handlers are needed to show a useful mouse cursor in Firefox
		window.addEventListener("dragover",function(e){
		  e = e || event;
		  e.preventDefault();
		  e.dataTransfer.dropEffect = 'none';
		},true);
		window.addEventListener("drop",function(e){
		  e = e || event;
		  e.preventDefault();
		},true);
	
		var dropDiv= document.getElementById("drop");
		dropDiv.ondrop  = this.drop.bind(this);
		dropDiv.ondragover  = this.allowDrop.bind(this);
	},
	appendControlElement: function(elmt) {
		var controls= document.getElementById("controls");
		controls.appendChild(elmt);  
		controls.appendChild(document.createTextNode(" "));  	// spacer
	},
	initDomElements: function() {				
		var play = document.createElement("BUTTON");
		play.id = "play";
		play.innerHTML= " &gt;";
		play.onclick = function(e){ this.resume(); }.bind(this);
		this.appendControlElement(play);  

		var pause = document.createElement("BUTTON");
		pause.id = "pause";
		pause.innerHTML= " ||";
		pause.onclick = function(e){ this.pause(); }.bind(this);	
		this.appendControlElement(pause);  

		var previous = document.createElement("BUTTON");
		previous.id = "previous";
		previous.innerHTML= " |&lt;&lt;";
		previous.onclick = this.playPreviousSong.bind(this);
		this.appendControlElement(previous);  

		var next = document.createElement("BUTTON");
		next.id = "next";
		next.innerHTML= " &gt;&gt;|";
		next.onclick = this.playNextSong.bind(this);
		this.appendControlElement(next);  

		var gain = document.createElement("input");
		gain.id = "gain";
		gain.name = "gain";
		gain.type = "range";
		gain.min = 0;
		gain.max = 255;
		gain.value = 255;
		gain.onchange = function(e){ this.setVolume(gain.value/255); }.bind(this);		
		this.appendControlElement(gain);  
						
		if (this._enableSeek) {
			var seek = document.createElement("input");
			seek.type = "range";
			seek.min = 0;
			seek.max = 255;
			seek.value = 0;
			seek.id = "seekPos";
			seek.name = "seekPos";
			// FF: 'onchange' triggers once the final value is selected; 
			// Chrome: already triggers while dragging; 'oninput' does not exist in IE 
			// but supposedly has the same functionality in Chrome & FF
			seek.oninput  = function(e){
				if (window.chrome)
					seek.blockUpdates= true;
			};
			seek.onchange  = function(e){
				if (!window.chrome)
					seek.onmouseup(e);
			};
			seek.onmouseup = function(e){	
				var p= ScriptNodePlayer.getInstance();
				this.seekPos(seek.value/255);
				seek.blockUpdates= false;
			}.bind(this);							
			this.appendControlElement(seek);  
		}
		if (this._enableSpeedTweak) {
			var speed = document.createElement("input");
			speed.type = "range";
			speed.min = 0;
			speed.max = 100;
			speed.value = 50;
			speed.id = "speed";
			speed.name = "speed";
			speed.onchange  = function(e){
				if (!window.chrome)
					speed.onmouseup(e);
			};
			
			speed.onmouseup = function(e){	
				var p= ScriptNodePlayer.getInstance();
				
				var tweak= 0.2; 			// allow 20% speed correction
				var f= (speed.value/50)-1;	// -1..1
				
				var s= p.getDefaultSampleRate();		
				s= Math.round(s*(1+(tweak*f)));
				p.resetSampleRate(s);
			}.bind(this);							
			this.appendControlElement(speed);  
		}
		
		this.initUserEngagement();
		this.initDrop();
		this.initTooltip();
		
		this.initExtensions();
	}
};
