/**
* Simple controls for ScriptNodePlayer.
*
*   version 1.03b
*
* This is a quick "ES6 class" port of the old legacy impl.
*
* Features an initial playlist, drag&drop of additional music files, and controls for "play", "pause",
* "next song", "previous song", "seek" (optional), "volume".
*
* This crude UI is not meant to be reusable but to show how the ScriptNodePlayer is used. ()
*/
class BasicControls {
	constructor(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current)
	{
		this._containerId = containerId;
		this._onNewTrackCallback = onNewTrackCallback;

		this._current = (typeof current != 'undefined') ? current : -1;

		if(Object.prototype.toString.call( songs ) === '[object Array]')
		{
			this._someSongs=  songs;
		}
		else
		{
			console.log("warning: no valid song list supplied.. starting empty");
			this._someSongs = [];
		}
		this._enableSeek = enableSeek;
		this._enableSpeedTweak = enableSpeedTweak;
		this._doParseUrl = doParseUrl;

		this._initDomElements();
	}

	// facade for player functionality so that BasicControls user does not also need to know the player
	pause() 				{ ScriptNodePlayer.getInstance().pause(); }
	resume() 				{ ScriptNodePlayer.getInstance().resume(); }
	setVolume(value) 		{ ScriptNodePlayer.getInstance().setVolume(value); }
	getSongInfo ()			{ return ScriptNodePlayer.getInstance().getSongInfo(); }

	selectSong(idx)
	{
		this._current = idx;
	}
	
	_addSong(filename)
	{
		
		this._someSongs.splice(this._current + 1, 0, filename);
	}

	seekPos(relPos)
	{
		let p = ScriptNodePlayer.getInstance();
		if (p)
		{
			p.seekPlaybackPosition(Math.round(p.getMaxPlaybackPosition() * relPos));
		}
	}

	_removeFromPlaylist(songname)
	{
		/* avoid additional complexity for now -
		if (this._someSongs[this._current] == songname) 
		{
			this._someSongs.splice(this._current, 1);
			if (this._current + 1 == this._someSongs.length) this._current = 0;
		}
		*/
	}

	playSongIdx(i) 
	{
		let p = ScriptNodePlayer.getInstance();
		if (p && this._someSongs.length) 
		{
			this._current = (i >=this._someSongs.length) ? 0 : i;
			let someSong = this._someSongs[this._current];
			this.playSong(someSong);
		}
	}

	playNextSong()
	{
		let p = ScriptNodePlayer.getInstance();
		if (p && this._someSongs.length)
		{
			this._current = (++this._current >= this._someSongs.length) ? 0 : this._current;

			let someSong = this._someSongs[this._current];
			this.playSong(someSong);
		}
	}

	playPreviousSong()
	{
		let p = ScriptNodePlayer.getInstance();
		if (p && this._someSongs.length)
		{
			this._current = (--this._current < 0) ? this._current + this._someSongs.length : this._current;

			let someSong = this._someSongs[this._current];
			this.playSong(someSong);
		}
	}

	_playSongWithBackand(options, onSuccess)
	{
		// backend adapter to be used has been explicitly specified
		let o = options.backendAdapter;

		ScriptNodePlayer.initialize(o.adapter, o.doOnTrackEnd, o.preload, o.enableSpectrum)
		.then((msg) => {
			onSuccess();
		});
	}

	playSong(someSong)
	{
		// handle Google's bullshit "autoplay policy"

		let audioCtx = ScriptNodePlayer.getWebAudioContext();
		if (audioCtx.state == "suspended")
		{
			let modal = document.getElementById('autoplayConfirm');
			modal.style.display = "block";		// force user to click

			window.globalDeferredPlay = function() {	// setup function to be used "onClick"
				audioCtx.resume();
				this._playSong(someSong);
			}.bind(this);
		}
		else
		{
			this._playSong(someSong);
		}
	}

	_loadSongFromURL(name, options)
	{
		let onFail = function() { this._removeFromPlaylist(name); }.bind(this);
		
		let onProgress = function(total, loaded){
				let c = loaded / total;		// songs may not advertise a total 		
				(c == 1 || total == 0) ? $('#spinner').hide() : $('#spinner').show();
			};
			
		ScriptNodePlayer.loadMusicFromURL(name, options, onFail, onProgress).then((status) => {
			this._onNewTrackCallback();
		});
	}

	_playSong(someSong)
	{
		let arr = this._doParseUrl(someSong);
		let url = arr[0];
		let options = arr[1];

		if (typeof options.backendAdapter != 'undefined')
		{
			let onPlayerReady = function() { this._loadSongFromURL(url, options); }.bind(this);
			this._playSongWithBackand(options, onPlayerReady);
		}
		else
		{
			this._loadSongFromURL(url, options);
		}
	}

	_updateSeekSlider()
	{
		// animate playback position slider
		let slider = document.getElementById("seekPos");
		if(slider && !slider.blockUpdates) {
			let p = ScriptNodePlayer.getInstance();
			if (p)
			{
				slider.value = Math.round(255 * p.getPlaybackPosition() / p.getMaxPlaybackPosition());
			}
		}
	}
	
	animate()
	{
		this._updateSeekSlider();
	}

	// ---------------------    drag&drop feature -----------------------------------

	_arrayify(f)
	{	
		// convert original FileList garbage into a regular array (once again those 
		// clueless morons where unable to design APIs that fit together..)
		
		let files = [];
		for (let i = 0; i < f.length; i++) {
			files.push(f[i]);	
		}
		return files;
	}
	
	_drop(ev)
	{
		ev.preventDefault();
		
		let data = ev.dataTransfer.getData("Text");
		let files = this._arrayify(ev.dataTransfer.files);
				
		if (ScriptNodePlayer.getInstance())
		{
			files.forEach((file) => {
				file.xname = "/tmp/" + file.name;	// hack needed since file.name has no setter
			});
			
			ScriptNodePlayer.loadFileData(files).then((result) => {
				
				result.forEach((filename) => {					
					this._addSong(filename);
				});
				this.playNextSong();
			});
		}
	}

	_initExtensions() {}	// to be overridden in subclass

	_allowDrop(ev)
	{
		ev.preventDefault();
		ev.dataTransfer.dropEffect = 'move'; 	// needed for FF
	}

	_initUserEngagement()
	{
		// handle Goggle's "autoplay policy" bullshit (patch the HTML/Script from here within this
		// lib so that the various existing html files need not be touched)

		let d = document.createElement("DIV");
		d.setAttribute("id", "autoplayConfirm");
		d.setAttribute("class", "modal-autoplay");

		let dc = document.createElement("DIV");
		dc.setAttribute("class", "modal-autoplay-content");

		let p = document.createElement("P");
		let t = document.createTextNode(
		"This page will auto play music. (You can white list this  \
		site to avoid this annoying dialog in the future.)\
		Click outside of this box to continue.");
		p.appendChild(t);

		dc.appendChild(p);
		d.appendChild(dc);

		document.body.insertBefore(d, document.body.firstChild);


		let s = document.createElement('script');
		s.text = 'var modal = document.getElementById("autoplayConfirm");\
			window.onclick = function(event) {\
				if (event.target == modal) {\
					modal.style.display = "none";\
					if (typeof window.globalDeferredPlay !== "undefined") { window.globalDeferredPlay();\
					delete window.globalDeferredPlay; }\
				}\
			}';
		document.body.appendChild(s);
	}

	_initTooltip()
	{
		let tooltipDiv = document.getElementById("tooltip");
		tooltipDiv.setAttribute("alt", `This is a hobby project, but it costs not only time to
regularly maintain this site but also money to pay for the internet service provider (etc). If
you want to keep this site up and running.. or if you just like my work and you'd like to see
more of it in the future, buy me a cup of coffee. Thank you!`);

		let f = document.createElement("form");
		f.setAttribute('method',"post");
		f.setAttribute('action',"https://www.paypal.com/cgi-bin/webscr");
		f.setAttribute('target',"_blank");

		let i1 = document.createElement("input");
		i1.type = "hidden";
		i1.value = "_s-xclick";
		i1.name = "cmd";
		f.appendChild(i1);

		let i2 = document.createElement("input");
		i2.type = "hidden";
		i2.value = "E7ACAHA7W5FYC";
		i2.name = "hosted_button_id";
		f.appendChild(i2);

		let i3 = document.createElement("input");
		i3.type = "image";
		i3.src = "stdlib/btn_donate_LG.gif";
		i3.className = "donate_btn";
		i3.border = "0";
		i3.name = "submit";
		i3.alt = "PayPal - The safer, easier way to pay online!";
		f.appendChild(i3);

		let i4 = document.createElement("img");
		i4.alt = "";
		i4.border = "0";
		i4.src = "stdlib/pixel.gif";
		i4.width = "1";
		i4.height = "1";
		f.appendChild(i4);

		tooltipDiv.appendChild(f);
	}

	_initDrop()
	{
		// the 'window' level handlers are needed to show a useful mouse cursor in Firefox
		window.addEventListener("dragover", function(e) {
		  e = e || event;
		  e.preventDefault();
		  e.dataTransfer.dropEffect = 'none';
		}, true);

		window.addEventListener("drop", function(e) {
		  e = e || event;
		  e.preventDefault();
		}, true);

		let dropDiv = document;	// no point to restrict drop to sub-elements..
		dropDiv.ondrop  = this._drop.bind(this);
		dropDiv.ondragover  = this._allowDrop.bind(this);
	}

	_appendControlElement(elmt)
	{
		let controls = document.getElementById(this._containerId);
		controls.appendChild(elmt);
		controls.appendChild(document.createTextNode(" "));  	// spacer
	}

	_addPlayerButton(id, label, onClick)
	{
		let btn = document.createElement("BUTTON");

		btn.id = id;
		btn.className = "playerBtn";
		btn.innerHTML = label;
		btn.onclick = onClick;

		this._appendControlElement(btn);
	}

	_addVolumeControl()
	{
		let gain = document.createElement("input");
		gain.id = "gain";
		gain.name = "gain";
		gain.type = "range";
		gain.min = 0;
		gain.max = 255;
		gain.value = 255;
		gain.onchange = function(e) { this.setVolume(gain.value / 255); }.bind(this);
		
		this._appendControlElement(gain);
	}

	_addSeekControl()
	{
		if (this._enableSeek)
		{
			let seek = document.createElement("input");
			seek.type = "range";
			seek.min = 0;
			seek.max = 255;
			seek.value = 0;
			seek.id = "seekPos";
			seek.name = "seekPos";
			// FF: 'onchange' triggers once the final value is selected;
			// Chrome: already triggers while dragging; 'oninput' does not exist in IE
			// but supposedly has the same functionality in Chrome & FF
			seek.oninput = function(e) {
				if (window.chrome)
					seek.blockUpdates = true;
			};
			seek.onchange = function(e) {
				if (!window.chrome)
					seek.onmouseup(e);
			};
			seek.onmouseup = function(e) {
				this.seekPos(seek.value / 255);
				seek.blockUpdates = false;
			}.bind(this);
			
			this._appendControlElement(seek);
		}
	}

	_addSpeedTweakControl()
	{
		if (this._enableSpeedTweak)
		{
			let speed = document.createElement("input");
			speed.type = "range";
			speed.min = 0;
			speed.max = 100;
			speed.value = 50;
			speed.id = "speed";
			speed.name = "speed";
			speed.onchange = function(e) {
				if (!window.chrome)
					speed.onmouseup(e);
			};

			speed.onmouseup = function(e) {
				let p= ScriptNodePlayer.getInstance();
				if (p)
				{
					let tweak = 0.2; 					// allow 20% speed correction
					let f = (speed.value / 50 ) - 1;	// -1..1

					let s = ScriptNodePlayer.getWebAudioSampleRate();
					s = Math.round(s * (1 + (tweak * f)));
					p.resetSampleRate(s);
				}
			}.bind(this);

			this._appendControlElement(speed);
		}
	}

	_initDomElements()
	{
		this._addPlayerButton("play", "<span class=\"material-icons\">&#57399;</span>",  function(e) { this.resume(); }.bind(this) );
		this._addPlayerButton("pause", "<span class=\"material-icons\">&#57396;</span>",  function(e) { this.pause(); }.bind(this) );
		this._addPlayerButton("previous", "<span class=\"material-icons\">&#57413;</span>",  this.playPreviousSong.bind(this) );
		this._addPlayerButton("next", "<span class=\"material-icons\">&#57412;</span>",  this.playNextSong.bind(this) );

		this._addVolumeControl();
		this._addSeekControl();
		this._addSpeedTweakControl();

		this._initUserEngagement();
		this._initDrop();
		this._initTooltip();

		this._initExtensions();
	}
};

