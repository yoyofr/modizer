let songs = [
		'Organya 2/Ama/obj0593-1.org',
		'Piston Collage Protected/Michi/obj0925-1.pttune',
	];

class PixelDisplayAccessor extends DisplayAccessor {
	constructor(doGetSongInfo)
	{
		super(doGetSongInfo);
	}

	getDisplayTitle()		{ return "PxTone+"; }
	getDisplaySubtitle() 	{ return "lets play some music.."; }
	getDisplayLine1() { return this.getSongInfo().title; }
	getDisplayLine2() { return this.getSongInfo().desc; }
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
		this._backend = new PixelBackendAdapter();
		this._backend.setProcessorBufSize(2048);

		ScriptNodePlayer.initialize(this._backend, this._doOnTrackEnd.bind(this), preloadFiles, true, undefined)
		.then((msg) => {

			let makeOptionsFromUrl = function(someSong) {
					let arr = someSong.split(";");
					let track = arr.length > 1 ? parseInt(arr[1]) : 0;
					someSong = arr[0];

					// drag&dropped temp files start with "/tmp/"
					let isLocal = someSong.startsWith("/tmp/") || someSong.startsWith("music/");
					someSong = isLocal ? someSong : window.location.protocol + "//ftp.modland.com/pub/modules/" + someSong;

					let options= {};
					options.track = track;

					return [someSong, options];
				};

			this._playerWidget = new BasicControls("controls", songs, makeOptionsFromUrl, this._doOnUpdate.bind(this), false, true);

			this._songDisplay = new SongDisplay(new PixelDisplayAccessor((function(){return this._playerWidget.getSongInfo();}.bind(this) )),
								[0x073ada, 0x5d82f9, 0x5d82f9, 0xf0f0f0, 0x5d82f9], 3, 0.5);

			this._playerWidget.playNextSong();
		});
	}
}