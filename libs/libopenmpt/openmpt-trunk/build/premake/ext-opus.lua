  
 project "opus"
  uuid "9a2d9099-e1a2-4287-b845-e3598ad24d70"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "opus"
  dofile "../../build/premake/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-opus"
  local extincludedirs = {
   "../../include/ogg/include",
	}
 	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
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
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  files {
   "../../include/opus/include/opus.h",
   "../../include/opus/include/opus_custom.h",
   "../../include/opus/include/opus_defines.h",
   "../../include/opus/include/opus_multistream.h",
   "../../include/opus/include/opus_types.h",
  }
  files {
   "../../include/opus/celt/*.c",
   "../../include/opus/celt/*.h",
   "../../include/opus/celt/x86/*.c",
   "../../include/opus/celt/x86/*.h",
   "../../include/opus/silk/*.c",
   "../../include/opus/silk/*.h",
   "../../include/opus/silk/float/*.c",
   "../../include/opus/silk/float/*.h",
   "../../include/opus/silk/x86/*.c",
   "../../include/opus/silk/x86/*.h",
   "../../include/opus/src/*.c",
   "../../include/opus/src/*.h",
  }
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
  filter { "kind:SharedLib" }
   defines { "DLL_EXPORT" }
  filter {}
