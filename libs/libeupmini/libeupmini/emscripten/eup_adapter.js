/*
 eup_adapter.js: Adapts EUP backend to generic WebAudio/ScriptProcessor player.

 version 1.1

 	Copyright (C) 2023 Juergen Wothke

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

class EUPBackendAdapter extends EmsHEAP16BackendAdapter {
	constructor()
	{
		super(backend_eup.Module, 2, new SimpleFileMapper(backend_eup.Module));
		
		this.ensureReadyNotification();
	}
	
	loadMusicData(sampleRate, path, filename, data, options)
	{
		filename = path + "/" + filename;

		let ret = this._loadMusicDataBuffer(filename, data, ScriptNodePlayer.getWebAudioSampleRate(), -999, false);						

		if (ret == 0)
		{
			this._setupOutputResampling(sampleRate);
		}
		return ret;
	}

	getSongInfoMeta()
	{
		return {
			title: String,
			artist: String,
		};
	}

	updateSongInfo(filename)
	{
		let result = this._songInfo;
		
		let numAttr = 2;
		let ret = this.Module.ccall('emu_get_track_info', 'number');

		let array = this.Module.HEAP32.subarray(ret>>2, (ret>>2)+numAttr);
		result.title= this._decodeBinaryToText(array[0], "shift-jis", 256);
		result.artist= this._decodeBinaryToText(array[1], "shift-jis", 256);
	}
};
