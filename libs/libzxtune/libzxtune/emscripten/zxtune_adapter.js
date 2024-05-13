/*
 zxtune_adapter.js: Adapts ZxTune backend to generic WebAudio/ScriptProcessor player.

   version 1.1
   copyright (C) 2015-2023 Juergen Wothke

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
class ZxTuneBackendAdapter extends EmsHEAP16BackendAdapter {
	constructor()
	{
		super(backend_ZxTune.Module, 2, new SimpleFileMapper(backend_ZxTune.Module));

		this.ensureReadyNotification();
	}

	loadMusicData(sampleRate, path, filename, data, options)
	{
		filename = this._getFilename(path, filename);

		let ret = this.Module.ccall('emu_load_file', 'number',
							['string', 'number', 'number', 'number', 'number', 'number'],
							[ filename, 0, 0, ScriptNodePlayer.getWebAudioSampleRate(), 1024, false]);

		if (ret == 0)
		{
			this._setupOutputResampling(sampleRate);
		}
		return ret;
	}

	evalTrackOptions(options)
	{
		super.evalTrackOptions(options);

		let id = (options && options.track) ? options.track : 0;
		return this.Module.ccall('emu_set_subsong', 'number', ['number'], [id]);
	}

	getSongInfoMeta()
	{
		return {
			title: String,
			author: String,
			desc: String,
			program: String,
			subPath: String,
			tracks: Number,
			currentTrack: Number
		};
	}

	updateSongInfo(filename)
	{
		let result = this._songInfo;
		
		let numAttr = 7;
		let ret = this.Module.ccall('emu_get_track_info', 'number');

		let array = this.Module.HEAP32.subarray(ret>>2, (ret>>2)+numAttr);
		result.title = this.Module.Pointer_stringify(array[0]);
		if (!result.title.length)
		{
			result.title = this._makeTitleFromPath(filename);
		}
		result.author = this.Module.Pointer_stringify(array[1]);
		result.desc = this.Module.Pointer_stringify(array[2]);
		result.program = this.Module.Pointer_stringify(array[3]);
		result.subPath = this.Module.Pointer_stringify(array[4]);

		let t = this.Module.Pointer_stringify(array[5]);
		result.tracks = parseInt(t);

		let ct = this.Module.Pointer_stringify(array[6]);
		result.currentTrack = parseInt(ct);
	}
};
