--
-- tests/actions/vstudio/vc2022/test_compile_settings.lua
-- Validate compiler settings in Visual Studio 2022 C/C++ projects.
-- Copyright (c) 2011-2021 Jason Perkins and the Premake project
--

local p = premake
local suite = test.declare("vstudio_vs2022_toolset_settings")
local vc2010 = p.vstudio.vc2010
local project = p.project

--
-- Setup
--

local wks, prj

function suite.setup()
	p.action.set("vs2022")
	wks, prj = test.createWorkspace()
end

local function prepare(platform)
	local cfg = test.getconfig(prj, "Debug", platform)
	vc2010.configurationProperties(cfg)
end

---
-- Check the default project settings
---

function suite.defaultSettings()
	prepare()
	test.capture [[
<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'" Label="Configuration">
	<ConfigurationType>Application</ConfigurationType>
	<UseDebugLibraries>false</UseDebugLibraries>
	<CharacterSet>Unicode</CharacterSet>
	<PlatformToolset>v143</PlatformToolset>
</PropertyGroup>
	]]
end

---
-- Check the project settings with the clang toolset
---

function suite.toolsetClang()
	toolset "clang"
	prepare()
	test.capture [[
<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'" Label="Configuration">
	<ConfigurationType>Application</ConfigurationType>
	<UseDebugLibraries>false</UseDebugLibraries>
	<CharacterSet>Unicode</CharacterSet>
	<PlatformToolset>ClangCL</PlatformToolset>
</PropertyGroup>
	]]
end
