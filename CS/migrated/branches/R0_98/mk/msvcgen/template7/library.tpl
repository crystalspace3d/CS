<?xml version="1.0" encoding = "Windows-1252"?>
<VisualStudioProject
	ProjectType="Visual C++"
	Version="7.00"
	Name="%project%"
	SccProjectName=""
	SccLocalPath="">
	<Platforms>
		<Platform
			Name="Win32"/>
	</Platforms>
	<Configurations>
		<Configuration
			Name="Release|Win32"
			OutputDirectory=".\csrelease\bin\libs"
			IntermediateDirectory=".\csrelease\temp\%project%"
			ConfigurationType="4"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="4"
				GlobalOptimizations="TRUE"
				InlineFunctionExpansion="2"
				EnableIntrinsicFunctions="TRUE"
				FavorSizeOrSpeed="1"
				OmitFramePointers="TRUE"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32"
				PreprocessorDefinitions="NDEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_STRICT_SMART_POINTERS"
				StringPooling="TRUE"
				RuntimeLibrary="2"
				EnableFunctionLevelLinking="TRUE"
				PrecompiledHeaderFile=".\csrelease\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csrelease\temp\%project%/"
				ObjectFile=".\csrelease\temp\%project%/"
				ProgramDataBaseFileName=".\csrelease\temp\%project%/%project%.pdb"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile=".\csrelease\bin\libs\%project%.lib"
				SuppressStartupBanner="TRUE"/>
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo File is a lib, Copy skipped."/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="NDEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32,%sourceroot%\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
		</Configuration>
		<Configuration
			Name="Debug|Win32"
			OutputDirectory=".\csdebug\bin\libs"
			IntermediateDirectory=".\csdebug\temp\%project%"
			ConfigurationType="4"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32"
				PreprocessorDefinitions="_DEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_DEBUG,CS_STRICT_SMART_POINTERS"
				MinimalRebuild="TRUE"
				RuntimeLibrary="3"
				RuntimeTypeInfo="TRUE"
				PrecompiledHeaderFile=".\csdebug\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csdebug\temp\%project%/"
				ObjectFile=".\csdebug\temp\%project%/"
				ProgramDataBaseFileName=".\csdebug\temp\%project%/%project%.pdb"
				BrowseInformation="1"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile="csdebug\bin\libs\%project%_d.lib"
				SuppressStartupBanner="TRUE"/>
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo File is a lib, Copy skipped."/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG,CS_DEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32,%sourceroot%\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
		</Configuration>
		<Configuration
			Name="ExtensiveMemDebug|Win32"
			OutputDirectory=".\csmemdbg\bin\libs"
			IntermediateDirectory=".\csmemdbg\temp\%project%"
			ConfigurationType="4"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32"
				PreprocessorDefinitions="_DEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_DEBUG,CS_EXTENSIVE_MEMDEBUG,CS_STRICT_SMART_POINTERS"
				MinimalRebuild="TRUE"
				RuntimeLibrary="3"
				RuntimeTypeInfo="TRUE"
				PrecompiledHeaderFile=".\csmemdbg\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csmemdbg\temp\%project%/"
				ObjectFile=".\csmemdbg\temp\%project%/"
				ProgramDataBaseFileName=".\csmemdbg\temp\%project%/%project%.pdb"
				BrowseInformation="1"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile="csmemdbg\bin\libs\%project%_d.lib"
				SuppressStartupBanner="TRUE"/>
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo File is a lib, Copy skipped."/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG,CS_DEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32,%sourceroot%\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
		</Configuration>
		<Configuration
			Name="Release_NR|Win32"
			OutputDirectory=".\csrelease\bin\libs"
			IntermediateDirectory=".\csrelease\temp\%project%"
			ConfigurationType="4"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="4"
				GlobalOptimizations="TRUE"
				InlineFunctionExpansion="2"
				EnableIntrinsicFunctions="TRUE"
				FavorSizeOrSpeed="1"
				OmitFramePointers="TRUE"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32"
				PreprocessorDefinitions="NDEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_STRICT_SMART_POINTERS;CS_USE_NEW_RENDERER"
				StringPooling="TRUE"
				RuntimeLibrary="2"
				EnableFunctionLevelLinking="TRUE"
				PrecompiledHeaderFile=".\csrelease\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csrelease\temp\%project%/"
				ObjectFile=".\csrelease\temp\%project%/"
				ProgramDataBaseFileName=".\csrelease\temp\%project%/%project%.pdb"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile=".\csrelease\bin\libs\%project%.lib"
				SuppressStartupBanner="TRUE"/>
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo File is a lib, Copy skipped."/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="NDEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32,%sourceroot%\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
		</Configuration>
		<Configuration
			Name="Debug_NR|Win32"
			OutputDirectory=".\csdebug\bin\libs"
			IntermediateDirectory=".\csdebug\temp\%project%"
			ConfigurationType="4"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32"
				PreprocessorDefinitions="_DEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_DEBUG,CS_STRICT_SMART_POINTERS;CS_USE_NEW_RENDERER"
				MinimalRebuild="TRUE"
				RuntimeLibrary="3"
				RuntimeTypeInfo="TRUE"
				PrecompiledHeaderFile=".\csdebug\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csdebug\temp\%project%/"
				ObjectFile=".\csdebug\temp\%project%/"
				ProgramDataBaseFileName=".\csdebug\temp\%project%/%project%.pdb"
				BrowseInformation="1"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile="csdebug\bin\libs\%project%_d.lib"
				SuppressStartupBanner="TRUE"/>
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo File is a lib, Copy skipped."/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG,CS_DEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="%sourceroot%\include\csutil\win32,%sourceroot%\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
		</Configuration>
	</Configurations>
	<Files>
		%groups%
	</Files>
	<Globals>
	</Globals>
</VisualStudioProject>
