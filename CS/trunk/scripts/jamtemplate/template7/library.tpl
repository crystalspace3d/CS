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
			OutputDirectory=".\release\temp\%project%"
			IntermediateDirectory=".\release\temp\%project%"
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
				AdditionalIncludeDirectories=""
				PreprocessorDefinitions="NDEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__"
				StringPooling="TRUE"
				RuntimeLibrary="2"
				EnableFunctionLevelLinking="TRUE"
				PrecompiledHeaderFile=".\release\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\release\temp\%project%/"
				ObjectFile=".\release\temp\%project%/"
				ProgramDataBaseFileName=".\release\temp\%project%/%project%.pdb"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="TRUE"
				AdditionalOptions="%lflags%"
				AdditionalDependencies="delayimp.lib %libs%"
				OutputFile="release\temp\%project%\%name%_vc7.dll"
				Version="4.0"
				LinkIncremental="1"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories=""
				IgnoreDefaultLibraryNames="LIBC,LIBCD,LIBCMT,LIBCMTD"
				DelayLoadDLLs="%delaylibs%"
				ProgramDatabaseFile=".\release\temp\%project%/%project%.pdb"
				SubSystem="2"
				OptimizeReferences="1"
				ImportLibrary=".\release\bin\libs\%project%.lib"
				TargetMachine="1"
				BaseAddress="" />
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to build root.
copy &quot;$(TargetPath)&quot; ..
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="NDEBUG"
				Culture="1033"
				AdditionalIncludeDirectories=""/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
		</Configuration>
		<Configuration
			Name="Debug|Win32"
			OutputDirectory=".\debug\temp\%project%"
			IntermediateDirectory=".\debug\temp\%project%"
			ConfigurationType="4"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%debugcflags%"
				AdditionalIncludeDirectories=""
				PreprocessorDefinitions="_DEBUG,_LIB,WIN32,_WINDOWS,WIN32_VOLATILE,__CRYSTAL_SPACE__,CS_DEBUG"
				MinimalRebuild="TRUE"
				RuntimeLibrary="3"
				RuntimeTypeInfo="TRUE"
				PrecompiledHeaderFile=".\debug\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\debug\temp\%project%/"
				ObjectFile=".\debug\temp\%project%/"
				ProgramDataBaseFileName=".\debug\temp\%project%/%project%.pdb"
				BrowseInformation="1"
				WarningLevel="3"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="TRUE"
				AdditionalOptions="%debuglflags%"
				AdditionalDependencies="delayimp.lib %debuglibs%"
				OutputFile="debug\temp\%project%\%name%_vc7d.dll"
				Version="1.0"
				LinkIncremental="2"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories=""
				IgnoreDefaultLibraryNames="LIBC,LIBCD,LIBCMTD"
				DelayLoadDLLs="%delaylibs%"
				GenerateDebugInformation="TRUE"
				ProgramDatabaseFile=".\debug\temp\%project%/%project%.pdb"
				SubSystem="2"
				ImportLibrary=".\debug\bin\libs/%project%_d.lib"
				TargetMachine="1"
				BaseAddress="" />
			<Tool
				Name="VCMIDLTool"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to build root.
copy &quot;$(TargetPath)&quot; ..
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG,CS_DEBUG"
				Culture="1033"
				AdditionalIncludeDirectories=""/>
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
