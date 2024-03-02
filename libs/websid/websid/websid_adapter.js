/*
 tinyrsid_adapter.js: Adapts WebSid (aka Tiny'R'Sid) backend to generic WebAudio/ScriptProcessor player.

 version 1.1

 	Copyright (C) 2020-2023 Juergen Wothke

 LICENSE

 This software is licensed under a CC BY-NC-SA
 (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
class SIDBackendAdapter extends EmsHEAP16BackendAdapter {
	constructor(basicROM, charROM, kernalROM, nextFrameCB, enableMd5)
	{
		super(backend_SID.Module, 2, new SimpleFileMapper(backend_SID.Module),
				new HEAP16ScopeProvider(backend_SID.Module, 0x8000));	// use stereo (for the benefit of multi-SID songs)

		this._scopeEnabled = false;

		this._ROM_SIZE = 0x2000;
		this._CHAR_ROM_SIZE = 0x1000;

		this._nextFrameCB = (typeof nextFrameCB == 'undefined') ? function() {} : nextFrameCB;

		this._basicROM = this._base64DecodeROM(basicROM, this._ROM_SIZE);
		this._charROM = this._base64DecodeROM(charROM, this._CHAR_ROM_SIZE);
		this._kernalROM = this._base64DecodeROM(kernalROM, this._ROM_SIZE);

		this._enableMd5 = (typeof enableMd5 == 'undefined') ? false : enableMd5;

		this._digiShownLabel = "";
		this._digiShownRate = 0;

		this._resetDigiMeta();

		this.ensureReadyNotification();
	}

	enableScope(enable)
	{
		this._scopeEnabled = enable;
	}

	computeAudioSamples()
	{
		if (typeof window.sid_measure_runs == 'undefined' || !window.sid_measure_runs)
		{
			window.sid_measure_sum = window.sid_measure_runs = 0;
		}
		this._nextFrameCB(this);	// used for "interactive mode"

		let t = performance.now();
//			console.profile(); // if compiled using "emcc.bat --profiling"

		if (this.Module.ccall('emu_compute_audio_samples', 'number'))
		{
			this._resetDigiMeta();
			return 1; // >0 means "end song"
		}
//			console.profileEnd();
		window.sid_measure_sum+= performance.now() - t;

		if (window.sid_measure_runs++ == 100)
		{
			window.sid_measure = window.sid_measure_sum / window.sid_measure_runs;

			window.sid_measure_sum = window.sid_measure_runs = 0;

			if (typeof window.sid_measure_avg_runs == 'undefined' || !window.sid_measure_avg_runs)
			{
				window.sid_measure_avg_sum = window.sid_measure;
				window.sid_measure_avg_runs = 1;
			}
			else
			{
				window.sid_measure_avg_sum += window.sid_measure;
				window.sid_measure_avg_runs += 1;
			}
			window.sid_measure_avg = window.sid_measure_avg_sum / window.sid_measure_avg_runs;
		}
		this._updateDigiMeta();
		return 0;
	}

	loadMusicData(sampleRate, path, filename, data, options)
	{
		this._setPanningConfig(this._panArray);

		let scopeEnabled = (typeof options.traceSID != 'undefined') ? options.traceSID : this._scopeEnabled;
		let procBufSize = this.getProcessorBufSize();

		let basicBuf= 0;
		if (this._basicROM) { basicBuf = this.Module._malloc(this._ROM_SIZE); this.Module.HEAPU8.set(this._basicROM, basicBuf);}

		let charBuf= 0;
		if (this._charROM) { charBuf = this.Module._malloc(this._CHAR_ROM_SIZE); this.Module.HEAPU8.set(this._charROM, charBuf);}

		let kernalBuf= 0;
		if (this._kernalROM) { kernalBuf = this.Module._malloc(this._ROM_SIZE); this.Module.HEAPU8.set(this._kernalROM, kernalBuf);}

		let buf = this.Module._malloc(data.length);
		this.Module.HEAPU8.set(data, buf);

		let ret = this.Module.ccall('emu_load_file', 'number',
							['string', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
							[ filename, buf, data.length, ScriptNodePlayer.getWebAudioSampleRate(), procBufSize, scopeEnabled, basicBuf, charBuf, kernalBuf, this._enableMd5]);

		this.Module._free(buf);

		if (kernalBuf) this.Module._free(kernalBuf);
		if (charBuf) this.Module._free(charBuf);
		if (basicBuf) this.Module._free(basicBuf);

		if (ret == 0)
		{
			this._setupOutputResampling(sampleRate);
		}
		return ret;
	}

	evalTrackOptions(options)
	{
		super.evalTrackOptions(options);

		this._resetDigiMeta();

		let track = (typeof options.track == 'undefined') ? -1 : options.track;
		return this.Module.ccall('emu_set_subsong', 'number', ['number'], [track]);
	}

	getSongInfoMeta()
	{
		return {
			loadAddr: Number,
			playSpeed: Number,
			maxSubsong: Number,
			actualSubsong: Number,
			songName: String,
			songAuthor: String,
			songReleased: String,
			md5: String				// optional: must be enabled via enableMd5 param
		};
	}

	updateSongInfo(filename)
	{
		let result = this._songInfo;
		
		let numAttr = 8;
		let ret = this.Module.ccall('emu_get_track_info', 'number');

		let array = this.Module.HEAP32.subarray(ret>>2, (ret>>2) + numAttr);

		result.loadAddr = this.Module.HEAP32[((array[0])>>2)]; // i32
		result.playSpeed = this.Module.HEAP32[((array[1])>>2)]; // i32
		result.maxSubsong = this.Module.HEAP8[(array[2])]; // i8
		result.actualSubsong = this.Module.HEAP8[(array[3])]; // i8
		result.songName = this._getExtAsciiString(array[4]);
		result.songAuthor = this._getExtAsciiString(array[5]);
		result.songReleased = this._getExtAsciiString(array[6]);
		result.md5 = this.Module.UTF8ToString(array[7]);
	}

	_resetDigiMeta()
	{
		this._digiTypes = {};
		this._digiRate = 0;
		this._digiBatches = 0;
		this._digiEmuCalls = 0;
	}

	_getExtAsciiString(heapPtr)
	{
		// Pointer_stringify cannot be used here since UTF-8 parsing
		// messes up original extASCII content
		let text = "";
		for (let j = 0; j < 32; j++) {
			let b = this.Module.HEAP8[heapPtr+j] & 0xff;
			if(b == 0) break;

			text += (b < 128) ? String.fromCharCode(b) : ("&#" + b + ";");
		}
		return text;
	}

	_base64DecodeROM(encoded, romSize)
	{
		if (typeof encoded == 'undefined') return 0;

		const binString = atob(encoded);
		let r = Uint8Array.from(binString, (m) => m.codePointAt(0));

		return (r.length == romSize) ? r : 0;
	}

	_updateDigiMeta()
	{
		// get a "not so jumpy" status describing digi output

		let dTypeStr = this._getExtAsciiString(this.Module.ccall('emu_get_digi_desc', 'number'));
		let dRate = this.Module.ccall('emu_get_digi_rate', 'number');
		// "emu_compute_audio_samples" correspond to 50/60Hz, i.e. to show some
		// status for at least half a second, labels should only be updated every
		// 25/30 calls..

		if (!isNaN(dRate) && dRate)
		{
			this._digiBatches++;
			this._digiRate += dRate;
			this._digiTypes[dTypeStr] = 1;
		}

		this._digiEmuCalls++;
		if (this._digiEmuCalls == 20)
		{
			this._digiShownLabel = "";

			if (!this._digiBatches)
			{
				this._digiShownRate = 0;
			}
			else
			{
				this._digiShownRate = Math.round(this._digiRate / this._digiBatches);

				const arr = Object.keys(this._digiTypes).sort();
				let c = 0;
				for (const key of arr) {
					if (key.length && (key != "NONE"))
						c++;
				}
				for (const key of arr) {
					if (key.length && (key != "NONE")  && ((c == 1) || (key != "D418")))	// ignore combinations with D418
						this._digiShownLabel+= (this._digiShownLabel.length ? "&" + key : key);
				}
			}
			this._resetDigiMeta();
		}
	}

// ---- postprocessing features

	_setPanningConfig(panArray)
	{
		if (typeof panArray != 'undefined')
		{
			this.Module.ccall('emu_set_panning_cfg',
								'number', ['number','number','number','number','number','number','number','number','number','number',
								'number','number','number','number','number','number','number','number','number','number',
								'number','number','number','number','number','number','number','number','number','number'],
								[panArray[0],panArray[1],panArray[2],panArray[3],panArray[4],panArray[5],panArray[6],panArray[7],panArray[8],panArray[9],
								panArray[10],panArray[11],panArray[12],panArray[13],panArray[14],panArray[15],panArray[16],panArray[17],panArray[18],panArray[19],
								panArray[20],panArray[21],panArray[22],panArray[23],panArray[24],panArray[25],panArray[26],panArray[27],panArray[28],panArray[29],]);
		}
	}

	/**
	* @param panArray 30-entry array with float-values ranging from 0.0 (100% left channel) to 1.0 (100% right channel) .. one value for each voice of the max 10 SIDs
	*/
	initPanningCfg(panArray)
	{
		if (panArray.length != 30)
		{
			console.log("error: initPanningCfg requires an array with panning-values for each of 10 SIDs that WebSid potentially supports.");
		}
		else
		{
			// note: this might be called before the WASM is ready
			this._panArray = panArray;

			if (!backend_SID.Module.notReady)
			{
				this._setPanningConfig(this._panArray);
			}
		}
	}

	getPanning(sidIdx, voiceIdx)
	{
		return this.Module.ccall('emu_get_panning', 'number', ['number', 'number'], [sidIdx, voiceIdx]);
	}

	setPanning(sidIdx, voiceIdx, panning)
	{
		this.Module.ccall('emu_set_panning', 'number',  ['number','number','number'], [sidIdx, voiceIdx, panning]);
	}

	getStereoLevel()
	{
		if (!this.isAdapterReady()) return 0;

		return this.Module.ccall('emu_get_stereo_level', 'number');
	}

	/**
	* @param effect_level -1=stereo completely disabled (no panning), 0=no stereo enhance disabled (only panning); >0= stereo enhance enabled: 16384=low 24576=medium 32767=high
	*/
	setStereoLevel(effect_level)
	{
		if (!this.isAdapterReady()) return 0;

		return this.Module.ccall('emu_set_stereo_level', 'number', ['number'], [effect_level]);
	}

	getReverbLevel()
	{
		if (!this.isAdapterReady()) return 0;

		return this.Module.ccall('emu_get_reverb', 'number');
	}

	/**
	* @param level 0..100
	*/
	setReverbLevel(level)
	{
		if (!this.isAdapterReady()) return 0;

		return this.Module.ccall('emu_set_reverb', 'number', ['number'], [level]);
	}

	getHeadphoneMode()
	{
		if (!this.isAdapterReady()) return 0;

		return this.Module.ccall('emu_get_headphone_mode', 'number');
	}

	/**
	* @param mode 0=headphone 1=ext-headphone
	*/
	setHeadphoneMode(mode)
	{
		if (!this.isAdapterReady()) return 0;

		return this.Module.ccall('emu_set_headphone_mode', 'number', ['number'], [mode]);
	}

// ---- 6581 SID filter configuration

	setFilterConfig6581(base, max, steepness, x_offset, distort, distortOffset, distortScale, distortThreshold, kink)
	{
		if (!this.isAdapterReady()) return;

		return this.Module.ccall('emu_set_filter_config_6581', 'number',
									['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
									[base, max, steepness, x_offset, distort, distortOffset, distortScale, distortThreshold, kink]);
	}

	getFilterConfig6581()
	{
		if (!this.isAdapterReady()) return {};

		let heapPtr = this.Module.ccall('emu_get_filter_cfg_6581', 'number') >> 3;	// 64-bit

		let result = {
			"base": this.Module.HEAPF64[heapPtr+0],
			"max": this.Module.HEAPF64[heapPtr+1],
			"steepness": this.Module.HEAPF64[heapPtr+2],
			"x_offset": this.Module.HEAPF64[heapPtr+3],
			"distort": this.Module.HEAPF64[heapPtr+4],
			"distortOffset": this.Module.HEAPF64[heapPtr+5],
			"distortScale": this.Module.HEAPF64[heapPtr+6],
			"distortThreshold": this.Module.HEAPF64[heapPtr+7],
			"kink": this.Module.HEAPF64[heapPtr+8],
		};
		return result;
	}

	getCutoffsLength()
	{
		return 1024;
	}

	fetchCutoffs6581(distortLevel, destinationArray)
	{
		if (!this.isAdapterReady()) return;
		let heapPtr = this.Module.ccall('emu_get_filter_cutoff_6581', 'number', ['number'], [distortLevel]) >> 3;	// 64-bit

		for (let i = 0; i < this.getCutoffsLength(); i++) {
			destinationArray[i] = this.Module.HEAPF64[heapPtr+i];
		}
	}

// ---- access to "historic" SID data

	/**
	* Gets a specific SID voice's output level (aka envelope) with about ~1 frame precison - using the actual position played
	* by the WebAudio infrastructure.
	*
	* prerequisite: ScriptNodePlayer must be configured with an "external ticker" for precisely timed access.
	*/
	readVoiceLevel(sidIdx, voiceIdx)
	{
		if (!this.isAdapterReady()) return 0;

		let p = ScriptNodePlayer.getInstance();
		let bufIdx = p.getTickToggle();
		let tick = p.getCurrentTick();

		return this.Module.ccall('emu_read_voice_level', 'number', ['number', 'number', 'number', 'number'], [sidIdx, voiceIdx, bufIdx, tick]);
	}

	/**
	* Gets a SID's register with about ~1 frame precison - using the actual position played
	* by the WebAudio infrastructure.
	*
	* prerequisite: ScriptNodePlayer must be configured with an "external ticker" for precisely timed access.
	*/
	getSIDRegister(sidIdx, reg)
	{
		if (!this.isAdapterReady()) return 0;

		let p = ScriptNodePlayer.getInstance();
		let bufIdx = p.getTickToggle();
		let tick = p.getCurrentTick();

		return this.Module.ccall('emu_get_sid_register', 'number', ['number', 'number', 'number', 'number'], [sidIdx, reg, bufIdx, tick]);
	}

// ---- manipulation of emulator state

	enableVoice(sidIdx, voice, on)
	{
		if (!this.isAdapterReady()) return;
		this.Module.ccall('emu_enable_voice', 'number', ['number', 'number', 'number'], [sidIdx, voice, on]);
	}

	setSIDRegister(sidIdx, reg, value)
	{
		if (!this.isAdapterReady()) return;
		return this.Module.ccall('emu_set_sid_register', 'number', ['number', 'number', 'number'], [sidIdx, reg, value]);
	}

	isSID6581()
	{
		if (!this.isAdapterReady()) return 0;
		return this.Module.ccall('emu_is_6581', 'number');
	}

	setSID6581(is6581)
	{
		if (!this.isAdapterReady()) return;
		this.Module.ccall('emu_set_6581', 'number', ['number'], [is6581]);
	}

	isNTSC()
	{
		if (!this.isAdapterReady()) return 0;
		return this.Module.ccall('emu_is_ntsc', 'number');
	}

	setNTSC(ntsc)
	{
		if (!this.isAdapterReady()) return;
		this.Module.ccall('emu_set_ntsc', 'number', ['number'], [ntsc]);
	}

	countSIDs()
	{
		if (!this.isAdapterReady()) return 0;
		return this.Module.ccall('emu_sid_count', 'number');
	}

	getSIDBaseAddr(sidIdx)
	{
		if (!this.isAdapterReady()) return 0;
		return this.Module.ccall('emu_get_sid_base', 'number', ['number'], [sidIdx]);
	}

	getRAM(offset)
	{
		if (!this.isAdapterReady()) return 0;
		return this.Module.ccall('emu_read_ram', 'number', ['number'], [offset]);
	}

	setRAM(offset, value)
	{
		if (!this.isAdapterReady()) return;
		this.Module.ccall('emu_write_ram', 'number', ['number', 'number'], [offset, value]);
	}

// ---- digi playback status

	getDigiType()
	{
		if (!this.isAdapterReady()) return 0;
		return this.Module.ccall('emu_get_digi_type', 'number');
	}

	getDigiTypeDesc()
	{
		if (!this.isAdapterReady()) return 0;
		return this._digiShownLabel;
	}

	getDigiRate()
	{
		if (!this.isAdapterReady()) return 0;
		return this._digiShownRate;
	}

// ---- util for debugging

	printMemDump(name, startAddr, endAddr)
	{
		let text = "const unsigned char "+name+"[] =\n{\n";
		let line = "";
		let j = 0;
		for (let i = 0; i < (endAddr - startAddr + 1); i++) {
			let d = this.Module.ccall('emu_read_ram', 'number', ['number'], [startAddr+i]);
			line += "0x"+(("00" + d.toString(16)).substr(-2).toUpperCase())+", ";
			if (j  == 11)
			{
				text += (line + "\n");
				line = "";
				j = 0;
			}
			else
			{
				j++;
			}
		}
		text += (j?(line+"\n"):"")+"}\n";
		console.log(text);
	}
};
