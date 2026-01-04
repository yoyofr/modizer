/*
 nez_adapter.js: Adapts NEZplug++ backend to generic WebAudio/ScriptProcessor player.

 version 1.1

 	Copyright (C) 2018-2023 Juergen Wothke


 Support for formats not normally handled by NEZplug++ was added within this
 adapter by means of conversion to KSS format (see this._usedConverter).


 LICENSE

 This library is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or (at
 your option) any later version. This library is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/
class NEZBackendAdapter extends EmsHEAP16BackendAdapter {
	constructor()
	{
		super(backend_NEZ.Module, 2, new SimpleFileMapper(backend_NEZ.Module));
	
		this._mus = new MusConverter(this);
		this._sng = new SngConverter();
		this._usedConverter = null;

		this.ensureReadyNotification();
	}

	convertFile(filename, data)
	{
		let fn = filename.toLowerCase();
		if(fn.endsWith(".mus"))
		{
			this._usedConverter = this._mus;
			return this._usedConverter.convert(filename, data);
		}
		else if(fn.endsWith(".sng"))
		{
			this._usedConverter = this._sng;
			return this._usedConverter.convert(filename, data);
		}

		// nothing to convert
		this._usedConverter = null;
		return [0, filename, data];
	}

	loadMusicData(sampleRate, path, filename, data, options)
	{
		filename = this._getFilename(path, filename);

		let ret = 0;

		[ret, filename, data] = this.convertFile(filename, data);
		if (ret == -1) { return ret; }	// file not ready.. retry later

		ret = this._loadMusicDataBuffer(filename, data, ScriptNodePlayer.getWebAudioSampleRate(), 1024, false);
		if (ret == 0)
		{
			this._setupOutputResampling(sampleRate);
		}
		return ret;
	}

	evalTrackOptions(options)
	{
		let timeout = (typeof options.timeout != 'undefined') ? options.timeout * 1000 : -1;
		ScriptNodePlayer.getInstance().setPlaybackTimeout(timeout);

		this.Module.ccall('emu_set_loop', 'number', ['number'], [(timeout>0)]);

		let id = (options && options.track) ? options.track : -1;	// by default do not set track
		return this.Module.ccall('emu_set_subsong', 'number', ['number'], [id]);
	}

	getSongInfoMeta()
	{
		return {
			title: String,
			artist: String,
			copyright: String,
			track: String,
			tracks: String,
			detail: String,
		};
	}

	updateSongInfo(filename)
	{
		let result = this._songInfo;
		
		// default KSS infos
		let numAttr = 5;
		let ret = this.Module.ccall('emu_get_track_info', 'number');

		let array = this.Module.HEAP32.subarray(ret>>2, (ret>>2)+numAttr);
		result.title = this.Module.Pointer_stringify(array[0]);
		if (!result.title.length) result.title = this._makeTitleFromPath(filename);

		result.artist = this.Module.Pointer_stringify(array[1]);
		result.copyright = this.Module.Pointer_stringify(array[2]);
		result.track = this.Module.Pointer_stringify(array[3]);
		result.tracks = this.Module.Pointer_stringify(array[4]);
		result.detail = this.Module.Pointer_stringify(array[5]);

		if (this._usedConverter)
		{
			this._usedConverter.updateSongInfo(result);
		}
	}
};

