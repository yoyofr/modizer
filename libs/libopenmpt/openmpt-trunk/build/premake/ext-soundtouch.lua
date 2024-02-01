 
 project "soundtouch"
  uuid "F5F8F6DE-84CF-4E9D-91EA-D9B5E2AA36CD"
  language "C++"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "shared"
  targetname "openmpt-soundtouch"
  includedirs { "../../include/soundtouch/include" }
	filter {}
  files {
   "../../include/soundtouch/include/BPMDetect.h",
   "../../include/soundtouch/include/FIFOSampleBuffer.h",
   "../../include/soundtouch/include/FIFOSamplePipe.h",
   "../../include/soundtouch/include/SoundTouch.h",
   "../../include/soundtouch/include/STTypes.h",
  }
  files {
   "../../include/soundtouch/source/SoundTouch/AAFilter.cpp",
   "../../include/soundtouch/source/SoundTouch/BPMDetect.cpp",
   "../../include/soundtouch/source/SoundTouch/cpu_detect_x86.cpp",
   "../../include/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp",
   "../../include/soundtouch/source/SoundTouch/FIRFilter.cpp",
   "../../include/soundtouch/source/SoundTouch/InterpolateCubic.cpp",
   "../../include/soundtouch/source/SoundTouch/InterpolateLinear.cpp",
   "../../include/soundtouch/source/SoundTouch/InterpolateShannon.cpp",
   "../../include/soundtouch/source/SoundTouch/mmx_optimized.cpp",
   "../../include/soundtouch/source/SoundTouch/PeakFinder.cpp",
   "../../include/soundtouch/source/SoundTouch/RateTransposer.cpp",
   "../../include/soundtouch/source/SoundTouch/SoundTouch.cpp",
   "../../include/soundtouch/source/SoundTouch/sse_optimized.cpp",
   "../../include/soundtouch/source/SoundTouch/TDStretch.cpp",
  }
  files {
   "../../include/soundtouch/source/SoundTouch/AAFilter.h",
   "../../include/soundtouch/source/SoundTouch/cpu_detect.h",
   "../../include/soundtouch/source/SoundTouch/FIRFilter.h",
   "../../include/soundtouch/source/SoundTouch/InterpolateCubic.h",
   "../../include/soundtouch/source/SoundTouch/InterpolateLinear.h",
   "../../include/soundtouch/source/SoundTouch/InterpolateShannon.h",
   "../../include/soundtouch/source/SoundTouch/PeakFinder.h",
   "../../include/soundtouch/source/SoundTouch/RateTransposer.h",
   "../../include/soundtouch/source/SoundTouch/TDStretch.h",
  }
  files {
   "../../include/soundtouch/source/SoundTouchDLL/SoundTouchDLL.cpp",
   "../../include/soundtouch/source/SoundTouchDLL/SoundTouchDLL.h",
  }
	filter { "action:vs*" }
		buildoptions { "/wd4267" }
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd6262" } -- analyze
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-shift-op-parentheses",
				"-Wno-unused-but-set-variable",
				"-Wno-unused-const-variable",
				"-Wno-unused-variable",
			}
		end
	filter {}
  defines { "DLL_EXPORTS" }

function mpt_use_soundtouch ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include",
		}
	filter {}
	links {
		"soundtouch",
	}
	filter {}
end
