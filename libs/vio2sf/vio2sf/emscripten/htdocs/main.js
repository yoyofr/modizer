let songs = [
	"Nintendo DS Sound Format/Kenta Nagata/The Legend Of Zelda - Phantom Hourglass/001 title.mini2sf",
	"Nintendo DS Sound Format/Yoko Shimomura/Kingdom Hearts 358-2 Days/ntr-p-ykgj-000a.mini2sf",
	"Nintendo DS Sound Format/Go Ichinose/Pokemon HeartGold & SoulSilver/title screen.mini2sf",
	"Nintendo DS Sound Format/Go Ichinose/Pokemon HeartGold & SoulSilver/battle - ho-oh.mini2sf",
	"Nintendo DS Sound Format/Go Ichinose/Pokemon Platinum/ntr-cpuj-jpn-03fc.mini2sf",
	"Nintendo DS Sound Format/Kazumi Totaka/Animal Crossing - Wild World/ntr-admj-jpn-00a0.mini2sf",
	"Nintendo DS Sound Format/- unknown/Crash Bandicoot - Mind Over Mutant/seq-001.mini2sf",
	"Nintendo DS Sound Format/Andras Kover/Scooby-Doo! Who's Watching Who/ntr-ac6e-usa-000a.mini2sf",
	"Nintendo DS Sound Format/Richard Joseph/James Pond - Codename Robocod/ntr-ajpp-eur-0000.mini2sf",
	];

class DSDisplayAccessor extends DisplayAccessor {
	constructor(doGetSongInfo)
	{
		super(doGetSongInfo);
	}

	getDisplayTitle() 		{ return "webDS";}
	getDisplaySubtitle() 	{ return "DeSmuME in action";}
	getDisplayLine1() { return this.getSongInfo().title +" ("+this.getSongInfo().artist+")";}
	getDisplayLine2() { return this.getSongInfo().copyright; }
	getDisplayLine3() { return ""; }
};

class DSControls extends BasicControls {
	constructor(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current)
	{
		super(containerId, songs, doParseUrl, onNewTrackCallback, enableSeek, enableSpeedTweak, current);
	}

	_addSong(filename)
	{
		if (filename.indexOf(".2sflib") == -1)
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
		this._backend = new DsBackendAdapter();
	//	this._backend.setProcessorBufSize(2048);

		ScriptNodePlayer.initialize(this._backend, this._doOnTrackEnd.bind(this), preloadFiles, true, undefined)
		.then((msg) => {

			let makeOptionsFromUrl = function(someSong) {
					let isLocal = someSong.startsWith("/tmp/") || someSong.startsWith("music/");
					someSong = isLocal ? someSong : window.location.protocol + "//ftp.modland.com/pub/modules/" + someSong;
					return [someSong, {}];
				};

			this._playerWidget = new DSControls("controls", songs, makeOptionsFromUrl, this._doOnUpdate.bind(this), false, true);

			this._songDisplay = new SongDisplay(new DSDisplayAccessor((function(){return this._playerWidget.getSongInfo();}.bind(this) )),
								[0xd65631,0xd7e627,0x101010,0x101010], 1, 0.5);

			this._playerWidget.playNextSong();
		});
	}
}