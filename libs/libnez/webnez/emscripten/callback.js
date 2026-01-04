// additional emscripten *.js library must be in this dumbshit format..
mergeInto(LibraryManager.library, {
	// returns 0 means file is ready; -1 if file is not yet available
	ems_request_file: function(name) {
		// reminder: used EMSCRIPTEN compiler mode does not allow use of ES6 features here, also
		// the stupid optimizer will garble the function names unless they are
		// specified using string literals..

		var p = window["ScriptNodePlayer"]["getInstance"]();
		if (!p["isReady"]()) {
			window["console"]["log"]("error: ems_request_file not ready");
		}
		else {
			return p["_fileRequestCallback"](name);
		}
	},
});