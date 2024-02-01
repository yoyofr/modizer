  
 project "opus"
  uuid "9a2d9099-e1a2-4287-b845-e3598ad24d70"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-opus"
  includedirs {
   "../../include/opus/include",
   "../../include/opus/celt",
   "../../include/opus/silk",
   "../../include/opus/silk/float",
   "../../include/opus/src",
   "../../include/opus/win32",
   "../../include/opus",
  }
	filter {}
  files {
   "../../include/opus/include/opus.h",
   "../../include/opus/include/opus_custom.h",
   "../../include/opus/include/opus_defines.h",
   "../../include/opus/include/opus_multistream.h",
   "../../include/opus/include/opus_projection.h",
   "../../include/opus/include/opus_types.h",
  }
  files {
   "../../include/opus/celt/*.c",
   "../../include/opus/celt/*.h",
   "../../include/opus/silk/*.c",
   "../../include/opus/silk/*.h",
   "../../include/opus/silk/float/*.c",
   "../../include/opus/silk/float/*.h",
   "../../include/opus/src/*.c",
   "../../include/opus/src/*.h",
  }
	filter { "architecture:x86 or x86_64" }
	files {
		"../../include/opus/celt/x86/*.c",
		"../../include/opus/celt/x86/*.h",
		"../../include/opus/silk/x86/*.c",
		"../../include/opus/silk/x86/*.h",
	}
	filter {}
  excludes {
   "../../include/opus/celt/opus_custom_demo.c",
   "../../include/opus/src/opus_compare.c",
   "../../include/opus/src/opus_demo.c",
   "../../include/opus/src/repacketizer_demo.c",
  }
  defines { "HAVE_CONFIG_H" }
  links { }
  filter { "action:vs*" }
    buildoptions { "/wd4244", "/wd4334" }
  filter {}
  filter { "action:vs*" }
		buildoptions { "/wd6255", "/wd6297" } -- analyze
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-excess-initializers",
				"-Wno-macro-redefined",
			}
		end
	filter {}
  filter { "kind:SharedLib" }
   defines { "DLL_EXPORT" }
  filter {}
		if _OPTIONS["clang"] then
			defines { "FLOAT_APPROX" }
		end

function mpt_use_opus ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/opus/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/opus/include",
		}
	filter {}
	links {
		"opus",
	}
	filter {}
end
