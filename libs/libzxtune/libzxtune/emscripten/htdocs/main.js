let songs = [
		'music/disco.tfe',
		'music/Untitled.dst',
		'music/Platoon.ay;6',
		'music/Speccy2.txt',
		'music/Nostalgy.ftc',
		'music/INEEDREST.ts',
		'music/unbeliev.tfc',
		'music/Illusion.psg',
		'music/tsd.sqt',
		'music/C_IMPROV.txt',
		'music/Kurztech.ym',
		'music/mass_production.tfd',
		'music/Qjeta_AlwaysOnMyMind.ym',
		'music/balala.chi',
		'music/popcorn.dmm',
		'music/Big.dst',
		'music/INSTANT.D',
		'music/mortal.m',
		'music/Maskarad.sqd',
		'music/COMETDAY.str',
		'music/carillon.cop',
		'music/ducktales1_moon.tf0',
		'music/tmk.tf0',
		'music/Calamba.psm',
		'music/EPILOG.st1',
		'music/fgfg.tfe'
	];

class ZXTuneDisplayAccessor extends DisplayAccessor {
	constructor(doGetSongInfo)
	{
		super(doGetSongInfo);
	}

		getDisplayTitle() 		{ return "spectreZX";}
		getDisplaySubtitle() 	{ return "lets play some music..";}
		getDisplayLine1() { return this.getSongInfo().title;}
		getDisplayLine2() { return this.getSongInfo().program == 'undefined' ? "" : this.getSongInfo().program; }
		getDisplayLine3() { return ""; }
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
		this._backend = new ZxTuneBackendAdapter();

		ScriptNodePlayer.initialize(this._backend, this._doOnTrackEnd.bind(this), preloadFiles, true, undefined)
		.then((msg) => {

			let makeOptionsFromUrl = function(someSong) {
					let arr = someSong.split(";");
					let track = arr.length > 1 ? parseInt(arr[1]) : 0;
					someSong = arr[0];

					// drag&dropped temp files start with "/tmp/"
					let isLocal = someSong.startsWith("/tmp/") || someSong.startsWith("music/");
					someSong = isLocal ? someSong : window.location.protocol + "//ftp.modland.com/pub/modules/" + someSong;

					let options = {};
					options.track = track;
					return [someSong, options];
				};

			this._playerWidget = new BasicControls("controls", songs, makeOptionsFromUrl, this._doOnUpdate.bind(this), false, true);

			this._songDisplay = new SongDisplay(new ZXTuneDisplayAccessor((function(){return this._playerWidget.getSongInfo();}.bind(this) )),
								[0xdd020a,0xd9df04,0x169e1a,0x00c9cd,0x00087d], 1, 0.5);

			this._playerWidget.playNextSong();
		});
	}
}