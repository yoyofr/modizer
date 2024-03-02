// "fix-chrome-consolelog" (or else some idiot chrome versions may crash with "Illegal invokation")
(function(){ var c = window.console.log; window.console.log = function() {c.apply(window.console, arguments); };
window.printErr= window.console.log;
})();

// create separate namespace for all the Emscripten stuff.. otherwise naming clashes may occur especially when 
// optimizing using closure compiler..

window.spp_backend_state_SID= {
	locateFile: function(path, scriptDirectory) { return (typeof window.WASM_SEARCH_PATH == 'undefined') ? path : window.WASM_SEARCH_PATH + path; },
	notReady: true,
	adapterCallback: function(){}	// overwritten later	
};
window.spp_backend_state_SID["onRuntimeInitialized"] = function() {	// emscripten callback needed in case async init is used (e.g. for WASM)
	this.notReady= false;
	this.adapterCallback();
}.bind(window.spp_backend_state_SID);

var backend_SID = (function(Module) {
var a;a||(a=typeof Module !== 'undefined' ? Module : {});var l={},m;for(m in a)a.hasOwnProperty(m)&&(l[m]=a[m]);a.arguments=[];a.thisProgram="./this.program";a.quit=function(b,c){throw c;};a.preRun=[];a.postRun=[];var n=!1,p=!1,q=!1,r=!1;n="object"===typeof window;p="function"===typeof importScripts;q="object"===typeof process&&"function"===typeof require&&!n&&!p;r=!n&&!q&&!p;var u="";function v(b){return a.locateFile?a.locateFile(b,u):u+b}
if(q){u=__dirname+"/";var w,x;a.read=function(b,c){w||(w=require("fs"));x||(x=require("path"));b=x.normalize(b);b=w.readFileSync(b);return c?b:b.toString()};a.readBinary=function(b){b=a.read(b,!0);b.buffer||(b=new Uint8Array(b));assert(b.buffer);return b};1<process.argv.length&&(a.thisProgram=process.argv[1].replace(/\\/g,"/"));a.arguments=process.argv.slice(2);"undefined"!==typeof module&&(module.exports=a);process.on("uncaughtException",function(b){throw b;});process.on("unhandledRejection",y);
a.quit=function(b){process.exit(b)};a.inspect=function(){return"[Emscripten Module object]"}}else if(r)"undefined"!=typeof read&&(a.read=function(b){return read(b)}),a.readBinary=function(b){if("function"===typeof readbuffer)return new Uint8Array(readbuffer(b));b=read(b,"binary");assert("object"===typeof b);return b},"undefined"!=typeof scriptArgs?a.arguments=scriptArgs:"undefined"!=typeof arguments&&(a.arguments=arguments),"function"===typeof quit&&(a.quit=function(b){quit(b)});else if(n||p)p?u=
self.location.href:document.currentScript&&(u=document.currentScript.src),u=0!==u.indexOf("blob:")?u.substr(0,u.lastIndexOf("/")+1):"",a.read=function(b){var c=new XMLHttpRequest;c.open("GET",b,!1);c.send(null);return c.responseText},p&&(a.readBinary=function(b){var c=new XMLHttpRequest;c.open("GET",b,!1);c.responseType="arraybuffer";c.send(null);return new Uint8Array(c.response)}),a.readAsync=function(b,c,e){var d=new XMLHttpRequest;d.open("GET",b,!0);d.responseType="arraybuffer";d.onload=function(){200==
d.status||0==d.status&&d.response?c(d.response):e()};d.onerror=e;d.send(null)},a.setWindowTitle=function(b){document.title=b};var z=a.print||("undefined"!==typeof console?console.log.bind(console):"undefined"!==typeof print?print:null),A=a.printErr||("undefined"!==typeof printErr?printErr:"undefined"!==typeof console&&console.warn.bind(console)||z);for(m in l)l.hasOwnProperty(m)&&(a[m]=l[m]);l=void 0;function B(b){var c;c||(c=16);return Math.ceil(b/c)*c}
var aa={"f64-rem":function(b,c){return b%c},"debugger":function(){debugger}},C=!1;function assert(b,c){b||y("Assertion failed: "+c)}
var ca={stackSave:function(){D()},stackRestore:function(){E()},arrayToC:function(b){var c=F(b.length);ba.set(b,c);return c},stringToC:function(b){var c=0;if(null!==b&&void 0!==b&&0!==b){var e=(b.length<<2)+1;c=F(e);var d=c,g=G;if(0<e){e=d+e-1;for(var h=0;h<b.length;++h){var f=b.charCodeAt(h);if(55296<=f&&57343>=f){var k=b.charCodeAt(++h);f=65536+((f&1023)<<10)|k&1023}if(127>=f){if(d>=e)break;g[d++]=f}else{if(2047>=f){if(d+1>=e)break;g[d++]=192|f>>6}else{if(65535>=f){if(d+2>=e)break;g[d++]=224|f>>
12}else{if(2097151>=f){if(d+3>=e)break;g[d++]=240|f>>18}else{if(67108863>=f){if(d+4>=e)break;g[d++]=248|f>>24}else{if(d+5>=e)break;g[d++]=252|f>>30;g[d++]=128|f>>24&63}g[d++]=128|f>>18&63}g[d++]=128|f>>12&63}g[d++]=128|f>>6&63}g[d++]=128|f&63}}g[d]=0}}return c}},da={string:ca.stringToC,array:ca.arrayToC};
function ea(b){var c;if(0===c||!b)return"";for(var e=0,d,g=0;;){d=G[b+g>>0];e|=d;if(0==d&&!c)break;g++;if(c&&g==c)break}c||(c=g);d="";if(128>e){for(;0<c;)e=String.fromCharCode.apply(String,G.subarray(b,b+Math.min(c,1024))),d=d?d+e:e,b+=1024,c-=1024;return d}return fa(b)}var ha="undefined"!==typeof TextDecoder?new TextDecoder("utf8"):void 0;
function ia(b,c){for(var e=c;b[e];)++e;if(16<e-c&&b.subarray&&ha)return ha.decode(b.subarray(c,e));for(e="";;){var d=b[c++];if(!d)return e;if(d&128){var g=b[c++]&63;if(192==(d&224))e+=String.fromCharCode((d&31)<<6|g);else{var h=b[c++]&63;if(224==(d&240))d=(d&15)<<12|g<<6|h;else{var f=b[c++]&63;if(240==(d&248))d=(d&7)<<18|g<<12|h<<6|f;else{var k=b[c++]&63;if(248==(d&252))d=(d&3)<<24|g<<18|h<<12|f<<6|k;else{var t=b[c++]&63;d=(d&1)<<30|g<<24|h<<18|f<<12|k<<6|t}}}65536>d?e+=String.fromCharCode(d):(d-=
65536,e+=String.fromCharCode(55296|d>>10,56320|d&1023))}}else e+=String.fromCharCode(d)}}function fa(b){return ia(G,b)}"undefined"!==typeof TextDecoder&&new TextDecoder("utf-16le");var buffer,ba,G,H;function ja(){a.HEAP8=ba=new Int8Array(buffer);a.HEAP16=new Int16Array(buffer);a.HEAP32=H=new Int32Array(buffer);a.HEAPU8=G=new Uint8Array(buffer);a.HEAPU16=new Uint16Array(buffer);a.HEAPU32=new Uint32Array(buffer);a.HEAPF32=new Float32Array(buffer);a.HEAPF64=new Float64Array(buffer)}var I,J,K,L,M,N,O;
I=J=K=L=M=N=O=0;function ka(){y("Cannot enlarge memory arrays. Either (1) compile with  -s TOTAL_MEMORY=X  with X higher than the current value "+P+", (2) compile with  -s ALLOW_MEMORY_GROWTH=1  which allows increasing the size at runtime, or (3) if you want malloc to return NULL (0) instead of this abort, compile with  -s ABORTING_MALLOC=0 ")}var Q=a.TOTAL_STACK||5242880,P=a.TOTAL_MEMORY||16777216;P<Q&&A("TOTAL_MEMORY should be larger than TOTAL_STACK, was "+P+"! (TOTAL_STACK="+Q+")");
a.buffer?buffer=a.buffer:("object"===typeof WebAssembly&&"function"===typeof WebAssembly.Memory?(a.wasmMemory=new WebAssembly.Memory({initial:P/65536,maximum:P/65536}),buffer=a.wasmMemory.buffer):buffer=new ArrayBuffer(P),a.buffer=buffer);ja();function R(b){for(;0<b.length;){var c=b.shift();if("function"==typeof c)c();else{var e=c.g;"number"===typeof e?void 0===c.a?a.dynCall_v(e):a.dynCall_vi(e,c.a):e(void 0===c.a?null:c.a)}}}var la=[],ma=[],na=[],oa=[],pa=!1;
function qa(){var b=a.preRun.shift();la.unshift(b)}var S=0,T=null,U=null;a.preloadedImages={};a.preloadedAudios={};function V(b){return String.prototype.startsWith?b.startsWith("data:application/octet-stream;base64,"):0===b.indexOf("data:application/octet-stream;base64,")}
(function(){function b(){try{if(a.wasmBinary)return new Uint8Array(a.wasmBinary);if(a.readBinary)return a.readBinary(g);throw"both async and sync fetching of the wasm failed";}catch(ua){y(ua)}}function c(){return a.wasmBinary||!n&&!p||"function"!==typeof fetch?new Promise(function(c){c(b())}):fetch(g,{credentials:"same-origin"}).then(function(b){if(!b.ok)throw"failed to load wasm binary file at '"+g+"'";return b.arrayBuffer()}).catch(function(){return b()})}function e(b){function d(b){k=b.exports;
if(k.memory){b=k.memory;var c=a.buffer;b.byteLength<c.byteLength&&A("the new buffer in mergeMemory is smaller than the previous one. in native wasm, we should grow memory here");c=new Int8Array(c);(new Int8Array(b)).set(c);a.buffer=buffer=b;ja()}a.asm=k;a.usingWasm=!0;S--;a.monitorRunDependencies&&a.monitorRunDependencies(S);0==S&&(null!==T&&(clearInterval(T),T=null),U&&(b=U,U=null,b()))}function e(b){d(b.instance)}function h(b){c().then(function(b){return WebAssembly.instantiate(b,f)}).then(b,function(b){A("failed to asynchronously prepare wasm: "+
b);y(b)})}if("object"!==typeof WebAssembly)return A("no native wasm support detected"),!1;if(!(a.wasmMemory instanceof WebAssembly.Memory))return A("no native wasm Memory in use"),!1;b.memory=a.wasmMemory;f.global={NaN:NaN,Infinity:Infinity};f["global.Math"]=Math;f.env=b;S++;a.monitorRunDependencies&&a.monitorRunDependencies(S);if(a.instantiateWasm)try{return a.instantiateWasm(f,d)}catch(va){return A("Module.instantiateWasm callback failed with error: "+va),!1}a.wasmBinary||"function"!==typeof WebAssembly.instantiateStreaming||
V(g)||"function"!==typeof fetch?h(e):WebAssembly.instantiateStreaming(fetch(g,{credentials:"same-origin"}),f).then(e,function(b){A("wasm streaming compile failed: "+b);A("falling back to ArrayBuffer instantiation");h(e)});return{}}var d="websid.wast",g="websid.wasm",h="websid.temp.asm.js";V(d)||(d=v(d));V(g)||(g=v(g));V(h)||(h=v(h));var f={global:null,env:null,asm2wasm:aa,parent:a},k=null;a.asmPreload=a.asm;var t=a.reallocBuffer;a.reallocBuffer=function(b){if("asmjs"===wa)var c=t(b);else a:{var d=
a.usingWasm?65536:16777216;0<b%d&&(b+=d-b%d);d=a.buffer.byteLength;if(a.usingWasm)try{c=-1!==a.wasmMemory.grow((b-d)/65536)?a.buffer=a.wasmMemory.buffer:null;break a}catch(Ca){c=null;break a}c=void 0}return c};var wa="";a.asm=function(b,c){if(!c.table){b=a.wasmTableSize;void 0===b&&(b=1024);var d=a.wasmMaxTableSize;c.table="object"===typeof WebAssembly&&"function"===typeof WebAssembly.Table?void 0!==d?new WebAssembly.Table({initial:b,maximum:d,element:"anyfunc"}):new WebAssembly.Table({initial:b,
element:"anyfunc"}):Array(b);a.wasmTable=c.table}c.memoryBase||(c.memoryBase=a.STATIC_BASE);c.tableBase||(c.tableBase=0);c=e(c);assert(c,"no binaryen method succeeded.");return c}})();
var ra=[function(){console.log("info cannot be retrieved  from corrupt .mus file")},function(){console.log("FATAL ERROR: This BASIC song requires emulator to be configured with optional KERNAL ROM and BASIC ROM")},function(){console.log("FATAL ERROR: no free memory for driver")},function(){console.log("ERROR: PSID INIT hangs")},function(b){console.log("JAM 0:  $"+b.toString(16))}];I=1024;J=I+2344224;ma.push({g:function(){sa()}},{g:function(){ta()}});a.STATIC_BASE=I;a.STATIC_BUMP=2344224;J+=16;
var W=0;function X(){W+=4;return H[W-4>>2]}var xa={};function Y(b,c){W=c;try{var e=X(),d=X(),g=X();b=0;Y.c||(Y.c=[null,[],[]],Y.j=function(b,c){var d=Y.c[b];assert(d);0===c||10===c?((1===b?z:A)(ia(d,0)),d.length=0):d.push(c)});for(c=0;c<g;c++){for(var h=H[d+8*c>>2],f=H[d+(8*c+4)>>2],k=0;k<f;k++)Y.j(e,G[h+k]);b+=f}return b}catch(t){return"undefined"!==typeof FS&&t instanceof FS.b||y(t),-t.f}}function ya(b){return Math.pow(2,b)}var za=J;J=J+4+15&-16;O=za;K=L=B(J);M=K+Q;N=B(M);H[O>>2]=N;
a.wasmTableSize=90;a.wasmMaxTableSize=90;a.h={};
a.i={abort:y,enlargeMemory:function(){ka()},getTotalMemory:function(){return P},abortOnCannotGrowMemory:ka,___cxa_pure_virtual:function(){C=!0;throw"Pure virtual function called!";},___setErrNo:function(b){a.___errno_location&&(H[a.___errno_location()>>2]=b);return b},___syscall140:function(b,c){W=c;try{var e=xa.l();X();var d=X(),g=X(),h=X();FS.o(e,d,h);H[g>>2]=e.position;e.m&&0===d&&0===h&&(e.m=null);return 0}catch(f){return"undefined"!==typeof FS&&f instanceof FS.b||y(f),-f.f}},___syscall146:Y,
___syscall6:function(b,c){W=c;try{var e=xa.l();FS.close(e);return 0}catch(d){return"undefined"!==typeof FS&&d instanceof FS.b||y(d),-d.f}},_abort:function(){a.abort()},_emscripten_asm_const_i:function(b){return ra[b]()},_emscripten_asm_const_ii:function(b,c){return ra[b](c)},_emscripten_memcpy_big:function(b,c,e){G.set(G.subarray(c,c+e),b);return b},_llvm_exp2_f64:function(){return ya.apply(null,arguments)},_llvm_trap:function(){y("trap!")},DYNAMICTOP_PTR:O,STACKTOP:L};var Aa=a.asm(a.h,a.i,buffer);
a.asm=Aa;var ta=a.__GLOBAL__sub_I_sid_cpp=function(){return a.asm.__GLOBAL__sub_I_sid_cpp.apply(null,arguments)},sa=a.__GLOBAL__sub_I_wavegenerator_cpp=function(){return a.asm.__GLOBAL__sub_I_wavegenerator_cpp.apply(null,arguments)};a._emu_compute_audio_samples=function(){return a.asm._emu_compute_audio_samples.apply(null,arguments)};a._emu_enable_voice=function(){return a.asm._emu_enable_voice.apply(null,arguments)};
a._emu_get_audio_buffer=function(){return a.asm._emu_get_audio_buffer.apply(null,arguments)};a._emu_get_audio_buffer_length=function(){return a.asm._emu_get_audio_buffer_length.apply(null,arguments)};a._emu_get_current_position=function(){return a.asm._emu_get_current_position.apply(null,arguments)};a._emu_get_digi_desc=function(){return a.asm._emu_get_digi_desc.apply(null,arguments)};a._emu_get_digi_rate=function(){return a.asm._emu_get_digi_rate.apply(null,arguments)};
a._emu_get_digi_type=function(){return a.asm._emu_get_digi_type.apply(null,arguments)};a._emu_get_filter_cfg_6581=function(){return a.asm._emu_get_filter_cfg_6581.apply(null,arguments)};a._emu_get_filter_cutoff_6581=function(){return a.asm._emu_get_filter_cutoff_6581.apply(null,arguments)};a._emu_get_headphone_mode=function(){return a.asm._emu_get_headphone_mode.apply(null,arguments)};a._emu_get_max_position=function(){return a.asm._emu_get_max_position.apply(null,arguments)};
a._emu_get_panning=function(){return a.asm._emu_get_panning.apply(null,arguments)};a._emu_get_reverb=function(){return a.asm._emu_get_reverb.apply(null,arguments)};a._emu_get_sample_rate=function(){return a.asm._emu_get_sample_rate.apply(null,arguments)};a._emu_get_sid_base=function(){return a.asm._emu_get_sid_base.apply(null,arguments)};a._emu_get_sid_register=function(){return a.asm._emu_get_sid_register.apply(null,arguments)};
a._emu_get_stereo_level=function(){return a.asm._emu_get_stereo_level.apply(null,arguments)};a._emu_get_trace_streams=function(){return a.asm._emu_get_trace_streams.apply(null,arguments)};a._emu_get_track_info=function(){return a.asm._emu_get_track_info.apply(null,arguments)};a._emu_is_6581=function(){return a.asm._emu_is_6581.apply(null,arguments)};a._emu_is_ntsc=function(){return a.asm._emu_is_ntsc.apply(null,arguments)};a._emu_load_file=function(){return a.asm._emu_load_file.apply(null,arguments)};
a._emu_number_trace_streams=function(){return a.asm._emu_number_trace_streams.apply(null,arguments)};a._emu_read_ram=function(){return a.asm._emu_read_ram.apply(null,arguments)};a._emu_read_voice_level=function(){return a.asm._emu_read_voice_level.apply(null,arguments)};a._emu_seek_position=function(){return a.asm._emu_seek_position.apply(null,arguments)};a._emu_set_6581=function(){return a.asm._emu_set_6581.apply(null,arguments)};
a._emu_set_filter_config_6581=function(){return a.asm._emu_set_filter_config_6581.apply(null,arguments)};a._emu_set_headphone_mode=function(){return a.asm._emu_set_headphone_mode.apply(null,arguments)};a._emu_set_ntsc=function(){return a.asm._emu_set_ntsc.apply(null,arguments)};a._emu_set_panning=function(){return a.asm._emu_set_panning.apply(null,arguments)};a._emu_set_panning_cfg=function(){return a.asm._emu_set_panning_cfg.apply(null,arguments)};
a._emu_set_reverb=function(){return a.asm._emu_set_reverb.apply(null,arguments)};a._emu_set_sid_register=function(){return a.asm._emu_set_sid_register.apply(null,arguments)};a._emu_set_stereo_level=function(){return a.asm._emu_set_stereo_level.apply(null,arguments)};a._emu_set_subsong=function(){return a.asm._emu_set_subsong.apply(null,arguments)};a._emu_sid_count=function(){return a.asm._emu_sid_count.apply(null,arguments)};a._emu_teardown=function(){return a.asm._emu_teardown.apply(null,arguments)};
a._emu_write_ram=function(){return a.asm._emu_write_ram.apply(null,arguments)};a._free=function(){return a.asm._free.apply(null,arguments)};a._malloc=function(){return a.asm._malloc.apply(null,arguments)};var F=a.stackAlloc=function(){return a.asm.stackAlloc.apply(null,arguments)},E=a.stackRestore=function(){return a.asm.stackRestore.apply(null,arguments)},D=a.stackSave=function(){return a.asm.stackSave.apply(null,arguments)};a.dynCall_v=function(){return a.asm.dynCall_v.apply(null,arguments)};
a.dynCall_vi=function(){return a.asm.dynCall_vi.apply(null,arguments)};a.asm=Aa;a.ccall=function(b,c,e,d){var g=a["_"+b];assert(g,"Cannot call unknown function "+b+", make sure it is exported");var h=[];b=0;if(d)for(var f=0;f<d.length;f++){var k=da[e[f]];k?(0===b&&(b=D()),h[f]=k(d[f])):h[f]=d[f]}e=g.apply(null,h);e="string"===c?ea(e):"boolean"===c?!!e:e;0!==b&&E(b);return e};a.UTF8ToString=fa;U=function Ba(){a.calledRun||Z();a.calledRun||(U=Ba)};
function Z(){function b(){if(!a.calledRun&&(a.calledRun=!0,!C)){pa||(pa=!0,R(ma));R(na);if(a.onRuntimeInitialized)a.onRuntimeInitialized();if(a.postRun)for("function"==typeof a.postRun&&(a.postRun=[a.postRun]);a.postRun.length;){var b=a.postRun.shift();oa.unshift(b)}R(oa)}}if(!(0<S)){if(a.preRun)for("function"==typeof a.preRun&&(a.preRun=[a.preRun]);a.preRun.length;)qa();R(la);0<S||a.calledRun||(a.setStatus?(a.setStatus("Running..."),setTimeout(function(){setTimeout(function(){a.setStatus("")},1);
b()},1)):b())}}a.run=Z;function y(b){if(a.onAbort)a.onAbort(b);void 0!==b?(z(b),A(b),b=JSON.stringify(b)):b="";C=!0;throw"abort("+b+"). Build with -s ASSERTIONS=1 for more info.";}a.abort=y;if(a.preInit)for("function"==typeof a.preInit&&(a.preInit=[a.preInit]);0<a.preInit.length;)a.preInit.pop()();a.noExitRuntime=!0;Z();
  return {
	Module: Module,  // expose original Module
  };
})(window.spp_backend_state_SID);
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
