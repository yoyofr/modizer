let songs = [
		"MoonBlaster/Kid Cnoz (Hans Cnossen)/MB Musax 1/Theme from bark [50Hz].MBM",
		"MoonBlaster/FWS/Fireworks Music Disk 2/Screaming Wolf [50Hz].MBM",
		"MoonBlaster/Master Of Audio (Jan-Lieuwe Koopmans)/Synthe Sector II disk1/Synthe Sector II - Promo 1.MBM",
		"MoonBlaster/Master Of Audio (Jan-Lieuwe Koopmans)/SoepFiskje/Strawberry Spring [50Hz].MBM",
		"MoonBlaster/- unknown/Picturedisk-menu/BREAKOUT.MBM",
		"MoonBlaster/Furdie/Mystery Zone/You belong to me [50Hz].MBM",
		"MoonBlaster/Wolf (Maarten van Strien)/MB Musax 3/Electricity [50Hz].MBM",

		"FAC SoundTracker/BDD (Ben den Dulk)/Impact MuSiX 2/Apple 2 GS.MUS;0;60",

		"HES/Yuriko Keino/xevious.hes;2;7",
		"HES/Yuriko Keino/xevious.hes;3;11",
		"HES/- unknown/pastel lime.hes;4;20",
		"HES/- unknown/pastel lime.hes;3;87",
		"HES/Nobuyuki Ohnogi/galaga 88.hes;10;42",
		"HES/Jin Watanabe/battle lode runner.hes;2;60",

		// SGC (Sega Master System, Game Gear, and Colecovision)
		"music/G-3380.sgc;0;60",
		"music/G-3327.sgc;2;60",
	];


class NEZDisplayAccessor extends DisplayAccessor {
	constructor(doGetSongInfo)
	{
		super(doGetSongInfo);
	}

	getDisplayTitle() 	{ return "webNEZ";}
	getDisplaySubtitle() 	{ return "antique computer music";}
	getDisplayLine1() { return this.getSongInfo().title; }
	getDisplayLine2() { return this.getSongInfo().artist.length ? this.getSongInfo().artist :
										(this.getSongInfo().tracks == '1' ? '' : ("track "+this.getSongInfo().track)); }
	getDisplayLine3() { return this.getSongInfo().copyright; }
};


class NEZControls extends BasicControls {
	constructor(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current)
	{
		super(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current);
	}

	_addSong(filename)
	{
		let arr = filename.split(".");
		let ext = arr.pop().toLowerCase();

		// ignore Moonblaster and FAC Soundtracker drumkits
		if ((filename.indexOf(".mbk") == -1) && (filename.indexOf(".sm1") == -1) && (filename.indexOf(".sm2") == -1))
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

	run()
	{
		let preloadFiles = [];	// no need for preload

		// note: with WASM this may not be immediately ready
		this._backend = new NEZBackendAdapter();

		ScriptNodePlayer.initialize(this._backend, this._doOnTrackEnd.bind(this), preloadFiles, true, undefined)
		.then((msg) => {

			let makeOptionsFromUrl = function(someSong) {
					let arr = someSong.split(";");
					someSong = arr[0];
					let track = arr.length > 1 ? parseInt(arr[1]) : -1;
					let timeout = arr.length > 2 ? parseInt(arr[2]) : -1;

					let options= {};
					options.track = isNaN(track) ? -1 : track;
					options.timeout = isNaN(timeout) ? -1 : timeout;


					// drag&dropped temp files start with "/tmp/"
					let isLocal = someSong.startsWith("/tmp/") || someSong.startsWith("music/");
					someSong = isLocal ? someSong : window.location.protocol + "//ftp.modland.com/pub/modules/" + someSong;

					return [someSong, options];
				};

			this._playerWidget = new NEZControls("controls", songs, makeOptionsFromUrl, this._doOnUpdate.bind(this), false, true);

			this._songDisplay = new SongDisplay(new NEZDisplayAccessor((function(){return this._playerWidget.getSongInfo();}.bind(this) )),
								[0x505050,0xffffff,0x404040,0xffffff], 1, 0.5);

			this._playerWidget.playNextSong();
		});
	}
}