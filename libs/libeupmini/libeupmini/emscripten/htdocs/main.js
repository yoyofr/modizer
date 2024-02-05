let songs = [
		'Euphony/Takayama Minja (Yoshito Shimizu)/In My Mind ver 2/In My Mind ver 2.eup',
		'Euphony/Takayama Minja (Yoshito Shimizu)/Midsummer Starry Sky/MANATU.eup',
		'Euphony/N.Plug/FF8E The Man with The Machine Gun/FF8ETMWM.eup',
		'Euphony/Boryu/F.Couperin Mysterious Barrier (Speed up version).eup',
		'Euphony/Tyuuyake (F-Okeshi)/Kagayaki/Kagayaki.eup',
		'Euphony/NIX/CANON4/CANON4.eup',
		'Euphony/Ponkara/Centrifugal Gravity/PON29.eup',
		'Euphony/TwinkleSoft (Yoshiyuki Hashizume)/Ending/ENDING_A.eup',
		'Euphony/Fox/Passionate Ocean.eup',
		'Euphony/Pon (Tomohiko Koyama)/Chopin Fantasy Impromptu in C-sharp minor.eup',
		'Euphony/ITARO/Lollipop Triangle.eup',
		'Euphony/Ramune Sakurano/GYMNOPED/GYMNOPED.eup',
		'Euphony/Nairn (Neanderthal Yabuki)/Hot Kombucha/Hot Kombucha.eup',
		'Euphony/whoo/Long Afternoon on Earth/Long Afternoon on Earth.eup',
		'Euphony/ToYoZo (Naohiko Toyozo)/Traveler of Memories/BGM_03 Bach - Aria.eup',
		'Euphony/Org/FUBUKI.eup',
		'Euphony/Nairn (Neanderthal Yabuki)/Strange Chinese Hat/Strange Chinese Hat.eup',
		'Euphony/Saji-san/Canon.eup',
		'Euphony/T.M (T. Murachi)/Shout Shoot.eup',
		'Euphony/Gori 3 (Daisuke Hirokawa)/Study composition 1-1/MOV1TW01.eup',
		'Euphony/ToYoZo (Naohiko Toyozo)/Taiga of Sadness/Taiga of Sadness.eup',
		'Euphony/Zuniti (H.Imoto)/Yellow Pelil \'95/Yellow Pelil \'95.eup',
		'Euphony/- unknown/e-tude 001 - Adventure!/S_EU001.eup',
		'Euphony/Takayama Minja (Yoshito Shimizu)/April Rise/April Rise.eup',
	];

// Google translation stuff
window.title = "";
window.titleTranslated = "";


class EuphonyDisplayAccessor extends DisplayAccessor {
	constructor(doGetSongInfo)
	{
		super(doGetSongInfo);
	}

	getDisplayTitle() 		{ return "webEuphony";}
	getDisplaySubtitle() 	{ return "FM Towns nostalgia";}
	getDisplayLine1() { return this.getSongInfo().title; }
	getDisplayLine2() { return window.titleTranslated; }
	getDisplayLine3() { return ""; }
};

class EuphonyControls extends BasicControls {
	constructor(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current)
	{
		super(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current);
	}

	_addSong(filename)
	{
		if ((filename.indexOf(".fmb") == -1) && (filename.indexOf(".FMB") == -1) &&
			(filename.indexOf(".pmb") == -1) && (filename.indexOf(".PMB") == -1))
		{
			super._addSong(filename);
		}
	}
};

class Main {
	constructor()
	{
		this._backend;
		this._playerWidget;
		this._songDisplay;
	}

	_doOnUpdate()
	{
		if (typeof this._lastId != 'undefined')
		{
			window.cancelAnimationFrame(this._lastId);	// prevent duplicate chains
		}
		this._animate();


		// hack to automatically translate japanese titles
		window.titleTranslated = "";
		window.title = this._displayAccessor.getDisplayLine1();

		if (!window.title.toLowerCase().endsWith(".eup")) // no point translating DOS filenames
		{
			var div = document.getElementById("translated");
			div.innerHTML = window.title;

			this._triggerTranslation();
		}

		this._songDisplay.redrawSongInfo();
	}

	_animate()
	{
		this._songDisplay.redrawSpectrum();
		this._playerWidget.animate()

		this._lastId = window.requestAnimationFrame(this._animate.bind(this));
	}

	_doOnTrackEnd()
	{
		this._playerWidget.playNextSong();
	}

	_playSongIdx(i)
	{
		this._playerWidget.playSongIdx(i);
	}

	// Google translation stuff
	_triggerTranslation()
	{
		var a = document.querySelector("#google_translate_element select");	// see tInit()

		try {
			const $options = Array.from(a.options);
			$options[0].selected = false;	// dummy
		} catch(e) {}
		a.selectedIndex = 0;	// to japanese
		a.dispatchEvent(new Event('change'));
	}

	_cleanupString(s)
	{
		s = s.toLowerCase();

		let r = "";
		for(let i = 0; i < s.length; i++) {
			let c = s.charCodeAt(i);

			// map existing shift-jis codes to reguar ascii
			if ((c >= 0xff41) && (c <= 0xff81)) {	// regular latin lowercase letters letters
				c = c-0xff41 + 0x61;
			}
			if ((c >= 0xff10) && (c <= 0xff19)) {	// 0..9
				c = c-0xff10 + 0x30;
			}
			if ((c >= 0xff08) && (c <= 0xff09)) {	// (, )
				c = c-0xff08 + 0x28;
			}
			if ((c >= 0x2018) && (c <= 0x2019)) {	// -> '
				c = 0x27;
			}
			if ((c >= 0x201c) && (c <= 0x201d)) {	// -> "
				c = 0x22;
			}
			if (c == 0x2010) {	// -
				c = 0x2d;
			}
			if (c == 0xff0f) {	// /
				c = 0x2f;
			}

			// ignore spaces and special points
			if ((c == 0x20) || (c == 0x2e) || (c == 0x70) || (c == 0x3000)|| (c == 0x85)|| (c == 0x30fb)) continue;

			r += String.fromCharCode(c);
		}
		return r;
	}

	_isTranslated(strlang)
	{
		if ((strlang != window.title) && strlang)
		{
			// Google's translation may turn already english text into uppercase
			let s1 = this._cleanupString(strlang);
			let s2 = this._cleanupString(window.title);

			return !(s1.startsWith(s2))
		}
		return false;
	}

	_bindNotify()
	{
		let main = this;
		// get notification when Google Translation is ready
		$('#translated').bind("DOMSubtreeModified", function() {
			let val = $(this);
			let strlang = "" + val[0].innerText + "";
			if ((strlang != window.title) && main._isTranslated(strlang))
			{
				window.titleTranslated = strlang;

				// translation seems to have original appended..
				let idx = window.titleTranslated.indexOf(window.title);
				if (idx > -1)
				{
					window.titleTranslated = window.titleTranslated.substring(0, idx);
				}
				main._songDisplay.redrawSongInfo();
			}
		});
	}

	run()
	{
		let preloadFiles = [];	// no need for preload

		// note: with WASM this may not be immediately ready
		this._backend = new EUPBackendAdapter();

		ScriptNodePlayer.initialize(this._backend, this._doOnTrackEnd.bind(this), preloadFiles, true, undefined)
		.then((msg) => {

			let makeOptionsFromUrl = function(someSong) {
					let arr = someSong.split(";");
					someSong = arr[0];

					// drag&dropped temp files start with "/tmp/"
					let isLocal = someSong.startsWith("/tmp/") || someSong.startsWith("music/");
					someSong = isLocal ? someSong : window.location.protocol + "//ftp.modland.com/pub/modules/" + someSong;

					let track = arr.length > 1 ? parseInt(arr[1]) : -1;
					let timeout = arr.length > 2 ? parseInt(arr[2]) : -1;

					let options = {};
					options.track = isNaN(track) ? -1 : track;
					options.timeout = isNaN(timeout) ? -1 : timeout;
					return [someSong, options];
				};

			this._playerWidget = new EuphonyControls("controls", songs, makeOptionsFromUrl, this._doOnUpdate.bind(this), false, true);

			this._displayAccessor = new EuphonyDisplayAccessor((function(){return this._playerWidget.getSongInfo();}.bind(this) ));
			this._songDisplay = new SongDisplay(this._displayAccessor, [0x505050,0xffffff,0x404040,0xffffff], 1, 0.5);

			this._bindNotify();

			this._playerWidget.playNextSong();
		});
	}
}