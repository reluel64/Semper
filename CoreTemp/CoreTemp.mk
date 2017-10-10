##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=CoreTemp
ConfigurationName      :=Debug
WorkspacePath          :=D:/Semper_Workspace
ProjectPath            :=D:/Semper_Workspace/CoreTemp
IntermediateDirectory  :=$(WorkspacePath)/build/$(ProjectName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=amdv2
Date                   :=10/10/2017
CodeLitePath           :="C:/Program Files/CodeLite"
LinkerName             :=D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/g++.exe
SharedObjectLinkerName :=D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/g++.exe -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(WorkspacePath)/bin/$(ConfigurationName)/Extensions/$(ProjectName).dll
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="CoreTemp.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/windres.exe
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)$(WorkspacePath) 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)semper 
ArLibs                 :=  "semper" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)$(WorkspacePath)/SDK 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/ar.exe rcu
CXX      := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/g++.exe
CC       := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/gcc.exe
CXXFLAGS :=  -g $(Preprocessors)
CFLAGS   :=  -g $(Preprocessors)
ASFLAGS  := 
AS       := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
CompilerDir:=D:\mingw-w64\x86_64-7.1.0-posix-seh-rt_v5-rev0\mingw64\x86_64-w64-mingw32
CompilerInclude:=$(CompilerDir)\include
CompilerLib:=$(CompilerDir)\lib
Objects0=$(IntermediateDirectory)/src_coretemp.c$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(SharedObjectLinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)
	@$(MakeDirCommand) "D:\Semper_Workspace/.build-debug"
	@echo rebuilt > "D:\Semper_Workspace/.build-debug/CoreTemp"

MakeIntermediateDirs:
	@$(MakeDirCommand) "$(WorkspacePath)/build/$(ProjectName)"


$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "$(WorkspacePath)/build/$(ProjectName)"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_coretemp.c$(ObjectSuffix): src/coretemp.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/CoreTemp/src/coretemp.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_coretemp.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_coretemp.c$(PreprocessSuffix): src/coretemp.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_coretemp.c$(PreprocessSuffix) src/coretemp.c

##
## Clean
##
clean:
	$(RM) -r $(WorkspacePath)/build/$(ProjectName)/


