 
 project "miniz"
  uuid "B5E0C06B-8121-426A-8FFB-4293ECAAE29C"
  language "C"
  location ( "../../build/" .. mpt_projectpathname .. "/ext" )
  mpt_projectname = "miniz"
  dofile "../../build/premake/premake-defaults-LIB.lua"
  dofile "../../build/premake/premake-defaults.lua"
  dofile "../../build/premake/premake-defaults-winver.lua"
  targetname "openmpt-miniz"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
  files {
   "../../include/miniz/miniz.c",
  }
