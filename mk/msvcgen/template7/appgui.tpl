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
			Name="Debug|Win32"
			OutputDirectory=".\csdebug\temp\%project%"
			IntermediateDirectory=".\csdebug\temp\%project%"
			ConfigurationType="1"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE"
			CharacterSet="2">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="..\..\plugins,..\..,..\..\include\cssys\win32,..\..\include,..\..\libs,..\..\support,..\..\apps"
				PreprocessorDefinitions="_DEBUG,WIN32,_CONSOLE,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_DEBUG,CS_STRICT_SMART_POINTERS"
				MinimalRebuild="TRUE"
				RuntimeLibrary="3"
				PrecompiledHeaderFile=".\csdebug\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csdebug\temp\%project%/"
				ObjectFile=".\csdebug\temp\%project%/"
				ProgramDataBaseFileName=".\csdebug\temp\%project%/%project%.pdb"
				BrowseInformation="1"
				WarningLevel="4"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="TRUE"
				AdditionalOptions="%lflags%"
				AdditionalDependencies="%libs%"
				OutputFile="csdebug\temp\%project%\%target%"
				LinkIncremental="2"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories="..\..\libs\cssys\win32\libs"
				IgnoreDefaultLibraryNames="LIBC"
				GenerateDebugInformation="TRUE"
				ProgramDatabaseFile=".\csdebug\temp\%project%/%project%.pdb"
				SubSystem="1"
				TargetMachine="1"/>
			<Tool
				Name="VCMIDLTool"
				TypeLibraryName=".\csdebug\temp\%project%/%project%.tlb"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to CS root.
copy &quot;$(TargetPath)&quot;  ..\..
echo Moving output to MSVC Debug Bin.
copy &quot;$(TargetPath)&quot;  csdebug\bin
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG,CS_DEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="..\..\include\cssys\win32,..\..\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
			<Tool
				Name="VCWebDeploymentTool"/>
		</Configuration>
		<Configuration
			Name="Release|Win32"
			OutputDirectory=".\csrelease\temp\%project%"
			IntermediateDirectory=".\csrelease\temp\%project%"
			ConfigurationType="1"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE"
			CharacterSet="2">
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
				AdditionalIncludeDirectories="..\..,..\..\include\cssys\win32,..\..\include,..\..\libs,..\..\support,..\..\apps,..\..\plugins"
				PreprocessorDefinitions="NDEBUG,_WINDOWS,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_STRICT_SMART_POINTERS"
				RuntimeLibrary="2"
				PrecompiledHeaderFile=".\csrelease\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csrelease\temp\%project%/"
				ObjectFile=".\csrelease\temp\%project%/"
				ProgramDataBaseFileName=".\csrelease\temp\%project%/%project%.pdb"
				WarningLevel="4"
				SuppressStartupBanner="TRUE"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				AdditionalOptions="%lflags%"
				AdditionalDependencies="%libs%"
				OutputFile="csrelease\temp\%project%\%target%"
				LinkIncremental="1"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories="..\..\libs\cssys\win32\libs"
				IgnoreDefaultLibraryNames="LIBC"
				ProgramDatabaseFile=".\csrelease\temp\%project%/%project%.pdb"
				SubSystem="2"
				OptimizeReferences="1"
				TargetMachine="1"/>
			<Tool
				Name="VCMIDLTool"
				TypeLibraryName=".\csrelease\temp\%project%/%project%.tlb"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to CS root.
copy &quot;$(TargetPath)&quot;  ..\..
echo Moving output to MSVC Release Bin.
copy &quot;$(TargetPath)&quot;  csrelease\bin
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="NDEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="..\..\include\cssys\win32,..\..\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
			<Tool
				Name="VCWebDeploymentTool"/>
		</Configuration>
		<Configuration
			Name="ExtensiveMemDebug|Win32"
			OutputDirectory=".\csmemdbg\temp\%project%"
			IntermediateDirectory=".\csmemdbg\temp\%project%"
			ConfigurationType="1"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE"
			CharacterSet="2">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="..\..\plugins,..\..,..\..\include\cssys\win32,..\..\include,..\..\libs,..\..\support,..\..\apps"
				PreprocessorDefinitions="_DEBUG,WIN32,_CONSOLE,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_DEBUG,CS_EXTENSIVE_MEMDEBUG,CS_STRICT_SMART_POINTERS"
				MinimalRebuild="TRUE"
				RuntimeLibrary="3"
				PrecompiledHeaderFile=".\csmemdbg\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csmemdbg\temp\%project%/"
				ObjectFile=".\csmemdbg\temp\%project%/"
				ProgramDataBaseFileName=".\csmemdbg\temp\%project%/%project%.pdb"
				BrowseInformation="1"
				WarningLevel="4"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="TRUE"
				AdditionalOptions="%lflags%"
				AdditionalDependencies="%libs%"
				OutputFile="csmemdbg\temp\%project%\%target%"
				LinkIncremental="2"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories="..\..\libs\cssys\win32\libs"
				IgnoreDefaultLibraryNames="LIBC"
				GenerateDebugInformation="TRUE"
				ProgramDatabaseFile=".\csmemdbg\temp\%project%/%project%.pdb"
				SubSystem="1"
				TargetMachine="1"/>
			<Tool
				Name="VCMIDLTool"
				TypeLibraryName=".\csmemdbg\temp\%project%/%project%.tlb"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to CS root.
copy &quot;$(TargetPath)&quot;  ..\..
echo Moving output to MSVC Debug Bin.
copy &quot;$(TargetPath)&quot;  csmemdbg\bin
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG,CS_DEBUG"
				Culture="1033"
				AdditionalIncludeDirectories="..\..\include\cssys\win32,..\..\include"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
			<Tool
				Name="VCWebDeploymentTool"/>
		</Configuration>
	</Configurations>
	<Files>
%groups%
	</Files>
	<Globals>
	</Globals>
</VisualStudioProject>
