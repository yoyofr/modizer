
 project "foo_openmpt"
  uuid "749102d3-b183-420c-a7d7-d8ff343c1a0c"
  language "C++"
  location ( "../../build/" .. mpt_projectpathname )
  vpaths { ["*"] = "../../libopenmpt/" }
  mpt_projectname = "foo_openmpt"
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  local extincludedirs = {
   "../..",
   "../../include/foobar2000sdk",
  }
	filter { "action:vs*" }
		includedirs ( extincludedirs )
	filter { "not action:vs*" }
		sysincludedirs ( extincludedirs )
	filter {}
  includedirs {
   "../..",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../libopenmpt/foo_openmpt.cpp",
  }

	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resdefines {
			"MPT_BUILD_VER_FILENAME=\"" .. mpt_projectname .. ".dll\"",
			"MPT_BUILD_VER_FILEDESC=\"" .. mpt_projectname .. "\"",
		}
	filter { "action:vs*", "kind:SharedLib or ConsoleApp or WindowedApp" }
		resincludedirs {
			"$(IntDir)/svn_version",
			"../../build/svn_version",
			"$(ProjDir)/../../build/svn_version",
		}
		files {
			"../../libopenmpt/libopenmpt_version.rc",
		}
	filter { "action:vs*", "kind:SharedLib" }
		resdefines { "MPT_BUILD_VER_DLL" }
	filter { "action:vs*", "kind:ConsoleApp or WindowedApp" }
		resdefines { "MPT_BUILD_VER_EXE" }
	filter {}

  characterset "Unicode"
  links { "libopenmpt", "zlib", "vorbis", "ogg" }
	links { "pfc", "foobar2000_SDK", "foobar2000_sdk_helpers", "foobar2000_component_client", "../../include/foobar2000sdk/foobar2000/shared/shared.lib" }
  links { "delayimp" }
  linkoptions {
   "/DELAYLOAD:mf.dll",
   "/DELAYLOAD:mfplat.dll",
   "/DELAYLOAD:mfreadwrite.dll",
--   "/DELAYLOAD:mfuuid.dll", -- static library
   "/DELAYLOAD:propsys.dll",
  }
  filter {}
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  filter { "configurations:Release" }
   flags { "LinkTimeOptimization" }
  filter {}

 external "pfc"
  location "../../include/foobar2000sdk/pfc"
  uuid "EBFFFB4E-261D-44D3-B89C-957B31A0BF9C"
  kind "StaticLib"
  language "C++"

 external "foobar2000_SDK"
  location "../../include/foobar2000sdk/foobar2000/SDK"
  uuid "E8091321-D79D-4575-86EF-064EA1A4A20D"
  kind "StaticLib"
  language "C++"

 external "foobar2000_sdk_helpers"
  location "../../include/foobar2000sdk/foobar2000/helpers"
  uuid "EE47764E-A202-4F85-A767-ABDAB4AFF35F"
  kind "StaticLib"
  language "C++"

 external "foobar2000_component_client"
  location "../../include/foobar2000sdk/foobar2000/foobar2000_component_client"
  uuid "71AD2674-065B-48F5-B8B0-E1F9D3892081"
  kind "StaticLib"
  language "C++"
