##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=Sample
ConfigurationName      :=Debug
WorkspacePath          :=D:/Semper_Workspace
ProjectPath            :=D:/Semper_Workspace/Sample
IntermediateDirectory  :=$(WorkspacePath)/build/$(ProjectName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=amdv2
Date                   :=24/07/2018
CodeLitePath           :="C:/Program Files/CodeLite"
LinkerName             :=D:/mxe/usr/x86_64-w64-mingw32.static/bin/g++.exe
SharedObjectLinkerName :=D:/mxe/usr/x86_64-w64-mingw32.static/bin/g++.exe -shared -fPIC
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
ObjectsFileList        :="Sample.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=D:/mxe/usr/x86_64-w64-mingw32.static/bin/windres.exe
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
AR       := D:/mxe/usr/x86_64-w64-mingw32.static/bin/ar.exe rcu
CXX      := D:/mxe/usr/x86_64-w64-mingw32.static/bin/g++.exe
CC       := D:/mxe/usr/x86_64-w64-mingw32.static/bin/gcc.exe
CXXFLAGS :=  -g $(Preprocessors)
CFLAGS   :=  -g $(Preprocessors)
ASFLAGS  := 
AS       := D:/mxe/usr/x86_64-w64-mingw32.static/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
CompilerDir:=D:\mxe\usr\x86_64-w64-mingw32.static
CompilerInclude:=$(CompilerDir)\include
CompilerLib:=$(CompilerDir)\lib
Objects0=$(IntermediateDirectory)/sample.c$(ObjectSuffix) 



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
	@echo rebuilt > "D:\Semper_Workspace/.build-debug/Sample"

MakeIntermediateDirs:
	@$(MakeDirCommand) "$(WorkspacePath)/build/$(ProjectName)"


$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "$(WorkspacePath)/build/$(ProjectName)"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/sample.c$(ObjectSuffix): sample.c $(IntermediateDirectory)/sample.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Sample/sample.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sample.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sample.c$(DependSuffix): sample.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sample.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sample.c$(DependSuffix) -MM sample.c

$(IntermediateDirectory)/sample.c$(PreprocessSuffix): sample.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sample.c$(PreprocessSuffix) sample.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r $(WorkspacePath)/build/$(ProjectName)/


