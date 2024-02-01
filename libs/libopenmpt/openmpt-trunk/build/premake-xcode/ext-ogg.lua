 
 project "ogg"
  uuid "d8d5e11c-f959-49ef-b741-b3f6de52ded8"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "ogg"
  dofile "../../build/premake-xcode/premake-defaults-LIBorDLL.lua"
  dofile "../../build/premake-xcode/premake-defaults.lua"
  targetname "openmpt-ogg"
  externalincludedirs { "../../include/ogg/include" }
  files {
   "../../include/ogg/include/ogg/ogg.h",
   "../../include/ogg/include/ogg/os_types.h",
   "../../include/ogg/src/bitwise.c",
   "../../include/ogg/src/framing.c",
  }
