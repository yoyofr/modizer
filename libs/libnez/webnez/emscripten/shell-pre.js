// create separate namespace for all the Emscripten stuff.. otherwise naming clashes may occur especially when
// optimizing using closure compiler..
window.spp_backend_state_NEZ= {
	locateFile: function(path, scriptDirectory) { return (typeof window.WASM_SEARCH_PATH == 'undefined') ? path : window.WASM_SEARCH_PATH + path; },
	print: function(t) {
		// suppress annoying "source file" info
		setTimeout(console.log.bind(console, t));	// "lose" the original context
	},
	notReady: true,
	adapterCallback: function(){}	// overwritten later
};
window.spp_backend_state_NEZ["onRuntimeInitialized"] = function() {	// emscripten callback needed in case async init is used (e.g. for WASM)
	this.notReady= false;
	this.adapterCallback();
}.bind(window.spp_backend_state_NEZ);

var backend_NEZ = (function(Module) {