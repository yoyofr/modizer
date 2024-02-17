// "fix-chrome-consolelog" (or else some idiot chrome versions may crash with "Illegal invokation")
(function(){ var c = window.console.log; window.console.log = function() {c.apply(window.console, arguments); };
window.printErr= window.console.log;
})();

// create separate namespace for all the Emscripten stuff.. otherwise naming clashes may occur especially when 
// optimizing using closure compiler..

window.spp_backend_state_SID= {
	notReady: true,
	adapterCallback: function(){}	// overwritten later	
};
window.spp_backend_state_SID["onRuntimeInitialized"] = function() {	// emscripten callback needed in case async init is used (e.g. for WASM)
	this.notReady= false;
	this.adapterCallback();
}.bind(window.spp_backend_state_SID);

var backend_SID = (function(Module) {
var a;a||(a=typeof Module !== 'undefined' ? Module : {});var l={},m;for(m in a)a.hasOwnProperty(m)&&(l[m]=a[m]);a.arguments=[];a.thisProgram="./this.program";a.quit=function(b,d){throw d;};a.preRun=[];a.postRun=[];var n=!1,p=!1,q=!1,r=!1;
if(a.ENVIRONMENT)if("WEB"===a.ENVIRONMENT)n=!0;else if("WORKER"===a.ENVIRONMENT)p=!0;else if("NODE"===a.ENVIRONMENT)q=!0;else if("SHELL"===a.ENVIRONMENT)r=!0;else throw Error("Module['ENVIRONMENT'] value is not valid. must be one of: WEB|WORKER|NODE|SHELL.");else n="object"===typeof window,p="function"===typeof importScripts,q="object"===typeof process&&"function"===typeof require&&!n&&!p,r=!n&&!q&&!p;
if(q){var t,u;a.read=function(b,d){t||(t=require("fs"));u||(u=require("path"));b=u.normalize(b);b=t.readFileSync(b);return d?b:b.toString()};a.readBinary=function(b){b=a.read(b,!0);b.buffer||(b=new Uint8Array(b));assert(b.buffer);return b};1<process.argv.length&&(a.thisProgram=process.argv[1].replace(/\\/g,"/"));a.arguments=process.argv.slice(2);"undefined"!==typeof module&&(module.exports=a);process.on("uncaughtException",function(b){if(!(b instanceof v))throw b;});process.on("unhandledRejection",
function(){process.exit(1)});a.inspect=function(){return"[Emscripten Module object]"}}else if(r)"undefined"!=typeof read&&(a.read=function(b){return read(b)}),a.readBinary=function(b){if("function"===typeof readbuffer)return new Uint8Array(readbuffer(b));b=read(b,"binary");assert("object"===typeof b);return b},"undefined"!=typeof scriptArgs?a.arguments=scriptArgs:"undefined"!=typeof arguments&&(a.arguments=arguments),"function"===typeof quit&&(a.quit=function(b){quit(b)});else if(n||p)a.read=function(b){var d=
new XMLHttpRequest;d.open("GET",b,!1);d.send(null);return d.responseText},p&&(a.readBinary=function(b){var d=new XMLHttpRequest;d.open("GET",b,!1);d.responseType="arraybuffer";d.send(null);return new Uint8Array(d.response)}),a.readAsync=function(b,d,e){var c=new XMLHttpRequest;c.open("GET",b,!0);c.responseType="arraybuffer";c.onload=function(){200==c.status||0==c.status&&c.response?d(c.response):e()};c.onerror=e;c.send(null)},"undefined"!=typeof arguments&&(a.arguments=arguments),a.setWindowTitle=
function(b){document.title=b};a.print="undefined"!==typeof console?console.log:"undefined"!==typeof print?print:null;a.printErr="undefined"!==typeof printErr?printErr:"undefined"!==typeof console&&console.warn||a.print;a.print=a.print;a.printErr=a.printErr;for(m in l)l.hasOwnProperty(m)&&(a[m]=l[m]);l=void 0;function w(b){var d;d||(d=16);return Math.ceil(b/d)*d}var x=0;function assert(b,d){b||y("Assertion failed: "+d)}
var E={stackSave:function(){z()},stackRestore:function(){A()},arrayToC:function(b){var d=B(b.length);C.set(b,d);return d},stringToC:function(b){var d=0;if(null!==b&&void 0!==b&&0!==b){var e=(b.length<<2)+1;d=B(e);var c=d,g=D;if(0<e){e=c+e-1;for(var h=0;h<b.length;++h){var f=b.charCodeAt(h);55296<=f&&57343>=f&&(f=65536+((f&1023)<<10)|b.charCodeAt(++h)&1023);if(127>=f){if(c>=e)break;g[c++]=f}else{if(2047>=f){if(c+1>=e)break;g[c++]=192|f>>6}else{if(65535>=f){if(c+2>=e)break;g[c++]=224|f>>12}else{if(2097151>=
f){if(c+3>=e)break;g[c++]=240|f>>18}else{if(67108863>=f){if(c+4>=e)break;g[c++]=248|f>>24}else{if(c+5>=e)break;g[c++]=252|f>>30;g[c++]=128|f>>24&63}g[c++]=128|f>>18&63}g[c++]=128|f>>12&63}g[c++]=128|f>>6&63}g[c++]=128|f&63}}g[c]=0}}return d}},aa={string:E.stringToC,array:E.arrayToC};
function ba(b){var d;if(0===d||!b)return"";for(var e=0,c,g=0;;){c=D[b+g>>0];e|=c;if(0==c&&!d)break;g++;if(d&&g==d)break}d||(d=g);c="";if(128>e){for(;0<d;)e=String.fromCharCode.apply(String,D.subarray(b,b+Math.min(d,1024))),c=c?c+e:e,b+=1024,d-=1024;return c}a:{d=D;for(e=b;d[e];)++e;if(16<e-b&&d.subarray&&F)b=F.decode(d.subarray(b,e));else for(e="";;){c=d[b++];if(!c){b=e;break a}if(c&128)if(g=d[b++]&63,192==(c&224))e+=String.fromCharCode((c&31)<<6|g);else{var h=d[b++]&63;if(224==(c&240))c=(c&15)<<
12|g<<6|h;else{var f=d[b++]&63;if(240==(c&248))c=(c&7)<<18|g<<12|h<<6|f;else{var k=d[b++]&63;if(248==(c&252))c=(c&3)<<24|g<<18|h<<12|f<<6|k;else{var J=d[b++]&63;c=(c&1)<<30|g<<24|h<<18|f<<12|k<<6|J}}}65536>c?e+=String.fromCharCode(c):(c-=65536,e+=String.fromCharCode(55296|c>>10,56320|c&1023))}else e+=String.fromCharCode(c)}}return b}var F="undefined"!==typeof TextDecoder?new TextDecoder("utf8"):void 0;"undefined"!==typeof TextDecoder&&new TextDecoder("utf-16le");var buffer,C,D,G,H;
function I(){a.HEAP8=C=new Int8Array(buffer);a.HEAP16=G=new Int16Array(buffer);a.HEAP32=H=new Int32Array(buffer);a.HEAPU8=D=new Uint8Array(buffer);a.HEAPU16=new Uint16Array(buffer);a.HEAPU32=new Uint32Array(buffer);a.HEAPF32=new Float32Array(buffer);a.HEAPF64=new Float64Array(buffer)}var K,L,M,N,O,P,Q,R;K=L=N=O=P=Q=R=0;M=!1;
function ca(){y("Cannot enlarge memory arrays. Either (1) compile with  -s TOTAL_MEMORY=X  with X higher than the current value "+S+", (2) compile with  -s ALLOW_MEMORY_GROWTH=1  which allows increasing the size at runtime, or (3) if you want malloc to return NULL (0) instead of this abort, compile with  -s ABORTING_MALLOC=0 ")}var T=a.TOTAL_STACK||5242880,S=a.TOTAL_MEMORY||16777216;S<T&&a.printErr("TOTAL_MEMORY should be larger than TOTAL_STACK, was "+S+"! (TOTAL_STACK="+T+")");
a.buffer?buffer=a.buffer:("object"===typeof WebAssembly&&"function"===typeof WebAssembly.Memory?(a.wasmMemory=new WebAssembly.Memory({initial:S/65536,maximum:S/65536}),buffer=a.wasmMemory.buffer):buffer=new ArrayBuffer(S),a.buffer=buffer);I();H[0]=1668509029;G[1]=25459;if(115!==D[2]||99!==D[3])throw"Runtime error: expected the system to be little-endian!";
function U(b){for(;0<b.length;){var d=b.shift();if("function"==typeof d)d();else{var e=d.f;"number"===typeof e?void 0===d.a?a.dynCall_v(e):a.dynCall_vi(e,d.a):e(void 0===d.a?null:d.a)}}}var da=[],ea=[],fa=[],ha=[],ia=[],ja=!1;function ka(){var b=a.preRun.shift();da.unshift(b)}var V=0,W=null,X=null;a.preloadedImages={};a.preloadedAudios={};function Y(b){return String.prototype.startsWith?b.startsWith("data:application/octet-stream;base64,"):0===b.indexOf("data:application/octet-stream;base64,")}
(function(){function b(){try{if(a.wasmBinary)return new Uint8Array(a.wasmBinary);if(a.readBinary)return a.readBinary(g);throw"on the web, we need the wasm binary to be preloaded and set on Module['wasmBinary']. emcc.py will do that for you when generating HTML (but not JS)";}catch(na){y(na)}}function d(){return a.wasmBinary||!n&&!p||"function"!==typeof fetch?new Promise(function(c){c(b())}):fetch(g,{credentials:"same-origin"}).then(function(b){if(!b.ok)throw"failed to load wasm binary file at '"+
g+"'";return b.arrayBuffer()}).catch(function(){return b()})}function e(b){function c(b){k=b.exports;if(k.memory){b=k.memory;var c=a.buffer;b.byteLength<c.byteLength&&a.printErr("the new buffer in mergeMemory is smaller than the previous one. in native wasm, we should grow memory here");c=new Int8Array(c);(new Int8Array(b)).set(c);a.buffer=buffer=b;I()}a.asm=k;a.usingWasm=!0;V--;a.monitorRunDependencies&&a.monitorRunDependencies(V);0==V&&(null!==W&&(clearInterval(W),W=null),X&&(b=X,X=null,b()))}function e(b){c(b.instance)}
function h(b){d().then(function(b){return WebAssembly.instantiate(b,f)}).then(b).catch(function(b){a.printErr("failed to asynchronously prepare wasm: "+b);y(b)})}if("object"!==typeof WebAssembly)return a.printErr("no native wasm support detected"),!1;if(!(a.wasmMemory instanceof WebAssembly.Memory))return a.printErr("no native wasm Memory in use"),!1;b.memory=a.wasmMemory;f.global={NaN:NaN,Infinity:Infinity};f["global.Math"]=Math;f.env=b;V++;a.monitorRunDependencies&&a.monitorRunDependencies(V);if(a.instantiateWasm)try{return a.instantiateWasm(f,
c)}catch(oa){return a.printErr("Module.instantiateWasm callback failed with error: "+oa),!1}a.wasmBinary||"function"!==typeof WebAssembly.instantiateStreaming||Y(g)||"function"!==typeof fetch?h(e):WebAssembly.instantiateStreaming(fetch(g,{credentials:"same-origin"}),f).then(e).catch(function(b){a.printErr("wasm streaming compile failed: "+b);a.printErr("falling back to ArrayBuffer instantiation");h(e)});return{}}var c="tinyrsid.wast",g="tinyrsid.wasm",h="tinyrsid.temp.asm.js";"function"===typeof a.locateFile&&
(Y(c)||(c=a.locateFile(c)),Y(g)||(g=a.locateFile(g)),Y(h)||(h=a.locateFile(h)));var f={global:null,env:null,asm2wasm:{"f64-rem":function(b,c){return b%c},"debugger":function(){debugger}},parent:a},k=null;a.asmPreload=a.asm;var J=a.reallocBuffer;a.reallocBuffer=function(b){if("asmjs"===pa)var c=J(b);else a:{var d=a.usingWasm?65536:16777216;0<b%d&&(b+=d-b%d);d=a.buffer.byteLength;if(a.usingWasm)try{c=-1!==a.wasmMemory.grow((b-d)/65536)?a.buffer=a.wasmMemory.buffer:null;break a}catch(ua){c=null;break a}c=
void 0}return c};var pa="";a.asm=function(b,c){if(!c.table){b=a.wasmTableSize;void 0===b&&(b=1024);var d=a.wasmMaxTableSize;c.table="object"===typeof WebAssembly&&"function"===typeof WebAssembly.Table?void 0!==d?new WebAssembly.Table({initial:b,maximum:d,element:"anyfunc"}):new WebAssembly.Table({initial:b,element:"anyfunc"}):Array(b);a.wasmTable=c.table}c.memoryBase||(c.memoryBase=a.STATIC_BASE);c.tableBase||(c.tableBase=0);(c=e(c))||y("no binaryen method succeeded. consider enabling more options, like interpreting, if you want that: https://github.com/kripken/emscripten/wiki/WebAssembly#binaryen-methods");
return c}})();K=1024;L=K+373696;ea.push({f:function(){la()}});a.STATIC_BASE=K;a.STATIC_BUMP=373696;L+=16;function ma(b){return Math.pow(2,b)}assert(!M);var qa=L;L=L+4+15&-16;R=qa;N=O=w(L);P=N+T;Q=w(P);H[R>>2]=Q;M=!0;a.wasmTableSize=1;a.wasmMaxTableSize=1;a.b={};
a.c={abort:y,enlargeMemory:function(){ca()},getTotalMemory:function(){return S},abortOnCannotGrowMemory:ca,___setErrNo:function(b){a.___errno_location&&(H[a.___errno_location()>>2]=b);return b},_emscripten_memcpy_big:function(b,d,e){D.set(D.subarray(d,d+e),b);return b},_llvm_exp2_f64:function(){return ma.apply(null,arguments)},DYNAMICTOP_PTR:R,STACKTOP:O};var ra=a.asm(a.b,a.c,buffer);a.asm=ra;var la=a.__GLOBAL__sub_I_sid_cpp=function(){return a.asm.__GLOBAL__sub_I_sid_cpp.apply(null,arguments)};
a._computeAudioSamples=function(){return a.asm._computeAudioSamples.apply(null,arguments)};a._enableVoices=function(){return a.asm._enableVoices.apply(null,arguments)};a._envIsNTSC=function(){return a.asm._envIsNTSC.apply(null,arguments)};a._envIsSID6581=function(){return a.asm._envIsSID6581.apply(null,arguments)};a._envSetNTSC=function(){return a.asm._envSetNTSC.apply(null,arguments)};a._envSetSID6581=function(){return a.asm._envSetSID6581.apply(null,arguments)};
a._free=function(){return a.asm._free.apply(null,arguments)};a._getBufferVoice1=function(){return a.asm._getBufferVoice1.apply(null,arguments)};a._getBufferVoice2=function(){return a.asm._getBufferVoice2.apply(null,arguments)};a._getBufferVoice3=function(){return a.asm._getBufferVoice3.apply(null,arguments)};a._getBufferVoice4=function(){return a.asm._getBufferVoice4.apply(null,arguments)};a._getMusicInfo=function(){return a.asm._getMusicInfo.apply(null,arguments)};
a._getRAM=function(){return a.asm._getRAM.apply(null,arguments)};a._getRegisterSID=function(){return a.asm._getRegisterSID.apply(null,arguments)};a._getSampleRate=function(){return a.asm._getSampleRate.apply(null,arguments)};a._getSoundBuffer=function(){return a.asm._getSoundBuffer.apply(null,arguments)};a._getSoundBufferLen=function(){return a.asm._getSoundBufferLen.apply(null,arguments)};a._loadSidFile=function(){return a.asm._loadSidFile.apply(null,arguments)};
a._malloc=function(){return a.asm._malloc.apply(null,arguments)};a._playTune=function(){return a.asm._playTune.apply(null,arguments)};var B=a.stackAlloc=function(){return a.asm.stackAlloc.apply(null,arguments)},A=a.stackRestore=function(){return a.asm.stackRestore.apply(null,arguments)},z=a.stackSave=function(){return a.asm.stackSave.apply(null,arguments)};a.dynCall_v=function(){return a.asm.dynCall_v.apply(null,arguments)};a.asm=ra;
a.ccall=function(b,d,e,c){var g=a["_"+b];assert(g,"Cannot call unknown function "+b+", make sure it is exported");var h=[];b=0;if(c)for(var f=0;f<c.length;f++){var k=aa[e[f]];k?(0===b&&(b=z()),h[f]=k(c[f])):h[f]=c[f]}e=g.apply(null,h);"string"===d&&(e=ba(e));0!==b&&A(b);return e};function v(b){this.name="ExitStatus";this.message="Program terminated with exit("+b+")";this.status=b}v.prototype=Error();v.prototype.constructor=v;var sa=null;X=function ta(){a.calledRun||Z();a.calledRun||(X=ta)};
function Z(){function b(){if(!a.calledRun&&(a.calledRun=!0,!x)){ja||(ja=!0,U(ea));U(fa);if(a.onRuntimeInitialized)a.onRuntimeInitialized();if(a.postRun)for("function"==typeof a.postRun&&(a.postRun=[a.postRun]);a.postRun.length;){var b=a.postRun.shift();ia.unshift(b)}U(ia)}}null===sa&&(sa=Date.now());if(!(0<V)){if(a.preRun)for("function"==typeof a.preRun&&(a.preRun=[a.preRun]);a.preRun.length;)ka();U(da);0<V||a.calledRun||(a.setStatus?(a.setStatus("Running..."),setTimeout(function(){setTimeout(function(){a.setStatus("")},
1);b()},1)):b())}}a.run=Z;a.exit=function(b,d){if(!d||!a.noExitRuntime||0!==b){if(!a.noExitRuntime&&(x=!0,O=void 0,U(ha),a.onExit))a.onExit(b);q&&process.exit(b);a.quit(b,new v(b))}};function y(b){if(a.onAbort)a.onAbort(b);void 0!==b?(a.print(b),a.printErr(b),b=JSON.stringify(b)):b="";x=!0;throw"abort("+b+"). Build with -s ASSERTIONS=1 for more info.";}a.abort=y;if(a.preInit)for("function"==typeof a.preInit&&(a.preInit=[a.preInit]);0<a.preInit.length;)a.preInit.pop()();a.noExitRuntime=!0;Z();
  return {
	Module: Module,  // expose original Module
  };
})(window.spp_backend_state_SID);
/*
 tinyrsid_adapter.js: Adapts Tiny'R'Sid backend to generic WebAudio/ScriptProcessor player.
 
 version 1.0
 
 	Copyright (C) 2015 Juergen Wothke

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
SIDBackendAdapter = (function(){ var $this = function () { 
		$this.base.call(this, backend_SID.Module, 1);
		this.playerSampleRate;
	}; 
	// TinyRSid's sample buffer contains 2-byte (signed short) sample data 
	// for 1 channel
	extend(EmsHEAP16BackendAdapter, $this, {
		getAudioBuffer: function() {
			var ptr=  this.Module.ccall('getSoundBuffer', 'number');			
			return ptr>>1;	// 16 bit samples			
		},
		getAudioBufferLength: function() {
			var len= this.Module.ccall('getSoundBufferLen', 'number');
			return len;
		},
		printMemDump: function(name, startAddr, endAddr) {	// util for debugging
			var text= "const unsigned char "+name+"[] =\n{\n";
			var line= "";
			var j= 0;
			for (var i= 0; i<(endAddr-startAddr+1); i++) {
				var d= this.Module.ccall('getRAM', 'number', ['number'], [startAddr+i]);
				line += "0x"+(("00" + d.toString(16)).substr(-2).toUpperCase())+", ";
				if (j  == 11) {						
					text+= (line + "\n");
					line= "";
					j= 0;
				}else {
					j++;
				}
			}		
			text+= (j?(line+"\n"):"")+"}\n";
			console.log(text);
		},
		computeAudioSamples: function() {
			var len= this.Module.ccall('computeAudioSamples', 'number');
			if (len <= 0) {			
				return 1; // >0 means "end song"
			}		
			return 0;	
		},
		getPathAndFilename: function(filename) {
			return ['/', filename];
		},
		registerFileData: function(pathFilenameArray, data) {
			return 0;	// FS not used in Tiny'R'Sid
		},
		loadMusicData: function(sampleRate, path, filename, data, options) {
			var buf = this.Module._malloc(data.length);
			this.Module.HEAPU8.set(data, buf);
			
			var isMus= filename.endsWith(".mus") || filename.endsWith(".str");	// Compute! Sidplayer file (stereo files not supported)			
			var ret = this.Module.ccall('loadSidFile', 'number', ['number', 'number', 'number'], [isMus, buf, data.length]);
			this.Module._free(buf);

			if (ret == 0) {
				this.playerSampleRate = this.Module.ccall('getSampleRate', 'number');
				this.resetSampleRate(sampleRate, this.playerSampleRate); 
			}
			return ret;			
		},
		evalTrackOptions: function(options) {
			if (typeof options.timeout != 'undefined') {
				ScriptNodePlayer.getInstance().setPlaybackTimeout(options.timeout*1000);
			}
			var traceSID= false;
			if (typeof options.traceSID != 'undefined') {
				traceSID= options.traceSID;
			}
			return this.Module.ccall('playTune', 'number', ['number', 'number'], [options.track, traceSID]);
		},
		teardown: function() {
			// nothing to do
		},
		getSongInfoMeta: function() {
			return {			
					loadAddr: Number,
					playSpeed: Number,
					maxSubsong: Number,
					actualSubsong: Number,
					songName: String,
					songAuthor: String, 
					songReleased: String 
					};
		},
		getExtAsciiString: function(heapPtr) {
			// Pointer_stringify cannot be used here since UTF-8 parsing 
			// messes up original extASCII content
			var text="";
			for (var j= 0; j<32; j++) {
				var b= this.Module.HEAP8[heapPtr+j] & 0xff;
				if(b ==0) break;
				
				if(b < 128){
					text = text + String.fromCharCode(b);
				} else {
					text = text + "&#" + b + ";";
				}
			}
			return text;
		},
		updateSongInfo: function(filename, result) {
		// get song infos (so far only use some top level module infos)
			var numAttr= 7;
			var ret = this.Module.ccall('getMusicInfo', 'number');
						
			var array = this.Module.HEAP32.subarray(ret>>2, (ret>>2)+7);
			result.loadAddr= this.Module.HEAP32[((array[0])>>2)]; // i32
			result.playSpeed= this.Module.HEAP32[((array[1])>>2)]; // i32
			result.maxSubsong= this.Module.HEAP8[(array[2])]; // i8
			result.actualSubsong= this.Module.HEAP8[(array[3])]; // i8
			result.songName= this.getExtAsciiString(array[4]);			
			result.songAuthor= this.getExtAsciiString(array[5]);
			result.songReleased= this.getExtAsciiString(array[6]);			
		},
		// for debugging.. disable voices (0-3) by clearing respective bit
		enableVoices: function(mask) {
			this.Module.ccall('enableVoices', 'number', ['number'], [mask]);
		},
		
		// C64 emu specific accessors (that might be useful in GUI)
		isSID6581: function() {
			return this.Module.ccall('envIsSID6581', 'number');
		},
		setSID6581: function(is6581) {
			this.Module.ccall('envSetSID6581', 'number', ['number'], [is6581]);
		},
		isNTSC: function() {
			return this.Module.ccall('envIsNTSC', 'number');
		},
		setNTSC: function(ntsc) {
			this.Module.ccall('envSetNTSC', 'number', ['number'], [ntsc]);
		},
		
		// To activate the below output a song must be started with the "traceSID" option set to 1:
		// At any given moment the below getters will then correspond to the output of getAudioBuffer
		// and what has last been generated by computeAudioSamples. They expose some of the respective
		// underlying internal SID state (the "filter" is NOT reflected in this data).
		getBufferVoice1: function() {
			var ptr=  this.Module.ccall('getBufferVoice1', 'number');			
			return ptr>>1;	// 16 bit samples			
		},
		getBufferVoice2: function() {
			var ptr=  this.Module.ccall('getBufferVoice2', 'number');			
			return ptr>>1;	// 16 bit samples			
		},
		getBufferVoice3: function() {
			var ptr=  this.Module.ccall('getBufferVoice3', 'number');			
			return ptr>>1;	// 16 bit samples			
		},
		getBufferVoice4: function() {
			var ptr=  this.Module.ccall('getBufferVoice4', 'number');			
			return ptr>>1;	// 16 bit samples			
		},		
		/**
		* This just queries the *current* state of the emulator. It
		* is less precisely correlated to the music that is currently playing (than the above
		* buffers), i.e. it represents the state *after* the last emulator call (respective data
		* may not yet have been fed to WebAudio or if it has already been fed then 
		* WebAudio may not yet be playing it yet). The lag should normally not be very large 
		* (<0.2s) and when using it for display purposes it would be hard to see a difference anyway.
		*/
		getRegisterSID: function(offset) {
			return this.Module.ccall('getRegisterSID', 'number', ['number'], [offset]);
		}
	});	return $this; })();
	