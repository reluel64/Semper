##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=Semper
ConfigurationName      :=Debug
WorkspacePath          :=D:/Semper_Workspace
ProjectPath            :=D:/Semper_Workspace/Semper
IntermediateDirectory  :=$(WorkspacePath)/build/$(ProjectName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=amdv2
Date                   :=14/08/2017
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
OutputFile             :=$(WorkspacePath)/bin/$(ConfigurationName)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)__USE_MINGW_ANSI_STDIO=1 $(PreprocessorSwitch)WIN32 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="Semper.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/windres.exe
LinkOptions            :=  -Wl,--out-implib=$(WorkspacePath)/SDK/libsemper.a
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)include $(IncludeSwitch)$(CompilerInclude)/pango-1.0 $(IncludeSwitch)$(CompilerInclude)/cairo $(IncludeSwitch)$(CompilerInclude)/freetype2 $(IncludeSwitch)$(CompilerInclude)/glib-2.0 $(IncludeSwitch)$(CompilerLib)/glib-2.0/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)Iphlpapi $(LibrarySwitch)curl $(LibrarySwitch)cairo $(LibrarySwitch)cairo-gobject $(LibrarySwitch)pangocairo-1.0 $(LibrarySwitch)gmodule-2.0 $(LibrarySwitch)gio-2.0 $(LibrarySwitch)imm32 $(LibrarySwitch)png16 $(LibrarySwitch)pangowin32-1.0 $(LibrarySwitch)pixman-1 $(LibrarySwitch)pangoft2-1.0 $(LibrarySwitch)shlwapi $(LibrarySwitch)z $(LibrarySwitch)dnsapi $(LibrarySwitch)usp10 $(LibrarySwitch)kernel32 $(LibrarySwitch)harfbuzz $(LibrarySwitch)freetype $(LibrarySwitch)glib-2.0 $(LibrarySwitch)pango-1.0 $(LibrarySwitch)gobject-2.0 $(LibrarySwitch)glib-2.0 $(LibrarySwitch)fontconfig $(LibrarySwitch)graphite2 $(LibrarySwitch)freetype $(LibrarySwitch)jpeg $(LibrarySwitch)iconv $(LibrarySwitch)expat $(LibrarySwitch)intl $(LibrarySwitch)ole32 $(LibrarySwitch)winmm $(LibrarySwitch)z $(LibrarySwitch)bz2 $(LibrarySwitch)pcre $(LibrarySwitch)ffi $(LibrarySwitch)iconv $(LibrarySwitch)ws2_32 $(LibrarySwitch)gdi32 $(LibrarySwitch)crypt32 $(LibrarySwitch)idn $(LibrarySwitch)intl $(LibrarySwitch)iconv $(LibrarySwitch)ssh2 $(LibrarySwitch)ssh2 $(LibrarySwitch)z $(LibrarySwitch)ws2_32 $(LibrarySwitch)intl $(LibrarySwitch)iconv $(LibrarySwitch)nettle $(LibrarySwitch)gnutls $(LibrarySwitch)hogweed $(LibrarySwitch)nettle $(LibrarySwitch)idn $(LibrarySwitch)z $(LibrarySwitch)ws2_32 $(LibrarySwitch)crypt32 $(LibrarySwitch)gmp $(LibrarySwitch)intl $(LibrarySwitch)iconv $(LibrarySwitch)wldap32 $(LibrarySwitch)z $(LibrarySwitch)ws2_32 $(LibrarySwitch)lua $(LibrarySwitch)wlanapi $(LibrarySwitch)gcc $(LibrarySwitch)gcrypt $(LibrarySwitch)gpg-error 
ArLibs                 :=  "Iphlpapi" "curl" "cairo" "cairo-gobject" "pangocairo-1.0" "gmodule-2.0" "gio-2.0" "imm32" "png16" "pangowin32-1.0" "pixman-1" "pangoft2-1.0" "shlwapi" "z" "dnsapi" "usp10" "kernel32" "harfbuzz" "freetype" "glib-2.0" "pango-1.0" "gobject-2.0" "glib-2.0" "fontconfig" "graphite2" "freetype" "jpeg" "iconv" "expat" "intl" "ole32" "winmm" "z" "bz2" "pcre" "ffi" "iconv" "ws2_32" "gdi32" "crypt32" "idn" "intl" "iconv" "ssh2" "ssh2" "z" "ws2_32" "intl" "iconv" "nettle" "gnutls" "hogweed" "nettle" "idn" "z" "ws2_32" "crypt32" "gmp" "intl" "iconv" "wldap32" "z" "ws2_32" "lua" "wlanapi" "gcc" "gcrypt" "gpg-error" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)$(CompilerLib) 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/ar.exe rcu
CXX      := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/g++.exe
CC       := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/gcc.exe
CXXFLAGS :=  -g -Wall $(Preprocessors)
CFLAGS   :=  -g3 -Wall -Wno-pointer-sign $(Preprocessors)
ASFLAGS  := 
AS       := D:/mingw-w64/x86_64-7.1.0-posix-seh-rt_v5-rev0/mingw64/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
CompilerDir:=D:\mingw-w64\x86_64-7.1.0-posix-seh-rt_v5-rev0\mingw64\x86_64-w64-mingw32
CompilerInclude:=$(CompilerDir)\include
CompilerLib:=$(CompilerDir)\lib
Objects0=$(IntermediateDirectory)/src_ancestor.c$(ObjectSuffix) $(IntermediateDirectory)/src_bind.c$(ObjectSuffix) $(IntermediateDirectory)/src_command.c$(ObjectSuffix) $(IntermediateDirectory)/src_diag.c$(ObjectSuffix) $(IntermediateDirectory)/src_enumerator.c$(ObjectSuffix) $(IntermediateDirectory)/src_event.c$(ObjectSuffix) $(IntermediateDirectory)/src_ini_parser.c$(ObjectSuffix) $(IntermediateDirectory)/src_math_parser.c$(ObjectSuffix) $(IntermediateDirectory)/src_mem.c$(ObjectSuffix) $(IntermediateDirectory)/src_mouse.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_parameter.c$(ObjectSuffix) $(IntermediateDirectory)/src_semper.c$(ObjectSuffix) $(IntermediateDirectory)/src_skeleton.c$(ObjectSuffix) $(IntermediateDirectory)/src_string_util.c$(ObjectSuffix) $(IntermediateDirectory)/src_surface.c$(ObjectSuffix) $(IntermediateDirectory)/src_surface_builtin.c$(ObjectSuffix) $(IntermediateDirectory)/src_team.c$(ObjectSuffix) $(IntermediateDirectory)/src_xpander.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_action.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_average.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_sources_calculator.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_disk.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_extension.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_folderinfo.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_folderview.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_input.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_iterator.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_memory.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_network.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_processor.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_sources_replacer.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_script.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_source.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_string.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_timed_action.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_time_s.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_webget.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_wifi.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_webget_reborn.c$(ObjectSuffix) $(IntermediateDirectory)/src_3rdparty_duktape.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_3rdparty_unicode_table.c$(ObjectSuffix) $(IntermediateDirectory)/src_crosswin_crosswin.c$(ObjectSuffix) $(IntermediateDirectory)/src_crosswin_win32.c$(ObjectSuffix) $(IntermediateDirectory)/src_crosswin_xlib.c$(ObjectSuffix) $(IntermediateDirectory)/src_image_image_cache.c$(ObjectSuffix) $(IntermediateDirectory)/src_image_image_cache_bmp.c$(ObjectSuffix) $(IntermediateDirectory)/src_image_image_cache_jpeg.c$(ObjectSuffix) $(IntermediateDirectory)/src_image_image_cache_png.c$(ObjectSuffix) $(IntermediateDirectory)/src_image_image_cache_svg.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_arc.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_objects_bar.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_button.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_histogram.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_image.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_line.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_object.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_spinner.c$(ObjectSuffix) $(IntermediateDirectory)/src_objects_string.c$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/src_sources_internal__diag_show.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_internal__surface_collector.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_sources_internal__surface_info.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_internal__surface_lister.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_script_js.c$(ObjectSuffix) $(IntermediateDirectory)/src_sources_script_lua.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

PostBuild:
	@echo Executing Post Build commands ...
	copy D:\Semper_Workspace\Semper\include\sources\extension.h D:\Semper_Workspace\SDK\extension.h
	@echo Done

MakeIntermediateDirs:
	@$(MakeDirCommand) "$(WorkspacePath)/build/$(ProjectName)"


$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "$(WorkspacePath)/build/$(ProjectName)"

PreBuild:
	@echo Executing Pre Build commands ...
	rmdir /s /q D:\Semper_Workspace\SDK
	mkdir D:\Semper_Workspace\SDK
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/src_ancestor.c$(ObjectSuffix): src/ancestor.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/ancestor.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ancestor.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ancestor.c$(PreprocessSuffix): src/ancestor.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ancestor.c$(PreprocessSuffix) src/ancestor.c

$(IntermediateDirectory)/src_bind.c$(ObjectSuffix): src/bind.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/bind.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_bind.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_bind.c$(PreprocessSuffix): src/bind.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_bind.c$(PreprocessSuffix) src/bind.c

$(IntermediateDirectory)/src_command.c$(ObjectSuffix): src/command.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/command.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_command.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_command.c$(PreprocessSuffix): src/command.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_command.c$(PreprocessSuffix) src/command.c

$(IntermediateDirectory)/src_diag.c$(ObjectSuffix): src/diag.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/diag.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_diag.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_diag.c$(PreprocessSuffix): src/diag.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_diag.c$(PreprocessSuffix) src/diag.c

$(IntermediateDirectory)/src_enumerator.c$(ObjectSuffix): src/enumerator.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/enumerator.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_enumerator.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_enumerator.c$(PreprocessSuffix): src/enumerator.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_enumerator.c$(PreprocessSuffix) src/enumerator.c

$(IntermediateDirectory)/src_event.c$(ObjectSuffix): src/event.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/event.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_event.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_event.c$(PreprocessSuffix): src/event.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_event.c$(PreprocessSuffix) src/event.c

$(IntermediateDirectory)/src_ini_parser.c$(ObjectSuffix): src/ini_parser.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/ini_parser.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ini_parser.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ini_parser.c$(PreprocessSuffix): src/ini_parser.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ini_parser.c$(PreprocessSuffix) src/ini_parser.c

$(IntermediateDirectory)/src_math_parser.c$(ObjectSuffix): src/math_parser.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/math_parser.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_math_parser.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_math_parser.c$(PreprocessSuffix): src/math_parser.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_math_parser.c$(PreprocessSuffix) src/math_parser.c

$(IntermediateDirectory)/src_mem.c$(ObjectSuffix): src/mem.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/mem.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mem.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mem.c$(PreprocessSuffix): src/mem.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mem.c$(PreprocessSuffix) src/mem.c

$(IntermediateDirectory)/src_mouse.c$(ObjectSuffix): src/mouse.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/mouse.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mouse.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mouse.c$(PreprocessSuffix): src/mouse.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mouse.c$(PreprocessSuffix) src/mouse.c

$(IntermediateDirectory)/src_parameter.c$(ObjectSuffix): src/parameter.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/parameter.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_parameter.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_parameter.c$(PreprocessSuffix): src/parameter.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_parameter.c$(PreprocessSuffix) src/parameter.c

$(IntermediateDirectory)/src_semper.c$(ObjectSuffix): src/semper.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/semper.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_semper.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_semper.c$(PreprocessSuffix): src/semper.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_semper.c$(PreprocessSuffix) src/semper.c

$(IntermediateDirectory)/src_skeleton.c$(ObjectSuffix): src/skeleton.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/skeleton.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_skeleton.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_skeleton.c$(PreprocessSuffix): src/skeleton.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_skeleton.c$(PreprocessSuffix) src/skeleton.c

$(IntermediateDirectory)/src_string_util.c$(ObjectSuffix): src/string_util.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/string_util.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_string_util.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_string_util.c$(PreprocessSuffix): src/string_util.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_string_util.c$(PreprocessSuffix) src/string_util.c

$(IntermediateDirectory)/src_surface.c$(ObjectSuffix): src/surface.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/surface.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_surface.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_surface.c$(PreprocessSuffix): src/surface.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_surface.c$(PreprocessSuffix) src/surface.c

$(IntermediateDirectory)/src_surface_builtin.c$(ObjectSuffix): src/surface_builtin.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/surface_builtin.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_surface_builtin.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_surface_builtin.c$(PreprocessSuffix): src/surface_builtin.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_surface_builtin.c$(PreprocessSuffix) src/surface_builtin.c

$(IntermediateDirectory)/src_team.c$(ObjectSuffix): src/team.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/team.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_team.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_team.c$(PreprocessSuffix): src/team.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_team.c$(PreprocessSuffix) src/team.c

$(IntermediateDirectory)/src_xpander.c$(ObjectSuffix): src/xpander.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/xpander.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_xpander.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_xpander.c$(PreprocessSuffix): src/xpander.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_xpander.c$(PreprocessSuffix) src/xpander.c

$(IntermediateDirectory)/src_sources_action.c$(ObjectSuffix): src/sources/action.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/action.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_action.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_action.c$(PreprocessSuffix): src/sources/action.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_action.c$(PreprocessSuffix) src/sources/action.c

$(IntermediateDirectory)/src_sources_average.c$(ObjectSuffix): src/sources/average.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/average.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_average.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_average.c$(PreprocessSuffix): src/sources/average.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_average.c$(PreprocessSuffix) src/sources/average.c

$(IntermediateDirectory)/src_sources_calculator.c$(ObjectSuffix): src/sources/calculator.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/calculator.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_calculator.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_calculator.c$(PreprocessSuffix): src/sources/calculator.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_calculator.c$(PreprocessSuffix) src/sources/calculator.c

$(IntermediateDirectory)/src_sources_disk.c$(ObjectSuffix): src/sources/disk.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/disk.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_disk.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_disk.c$(PreprocessSuffix): src/sources/disk.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_disk.c$(PreprocessSuffix) src/sources/disk.c

$(IntermediateDirectory)/src_sources_extension.c$(ObjectSuffix): src/sources/extension.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/extension.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_extension.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_extension.c$(PreprocessSuffix): src/sources/extension.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_extension.c$(PreprocessSuffix) src/sources/extension.c

$(IntermediateDirectory)/src_sources_folderinfo.c$(ObjectSuffix): src/sources/folderinfo.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/folderinfo.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_folderinfo.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_folderinfo.c$(PreprocessSuffix): src/sources/folderinfo.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_folderinfo.c$(PreprocessSuffix) src/sources/folderinfo.c

$(IntermediateDirectory)/src_sources_folderview.c$(ObjectSuffix): src/sources/folderview.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/folderview.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_folderview.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_folderview.c$(PreprocessSuffix): src/sources/folderview.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_folderview.c$(PreprocessSuffix) src/sources/folderview.c

$(IntermediateDirectory)/src_sources_input.c$(ObjectSuffix): src/sources/input.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/input.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_input.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_input.c$(PreprocessSuffix): src/sources/input.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_input.c$(PreprocessSuffix) src/sources/input.c

$(IntermediateDirectory)/src_sources_iterator.c$(ObjectSuffix): src/sources/iterator.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/iterator.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_iterator.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_iterator.c$(PreprocessSuffix): src/sources/iterator.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_iterator.c$(PreprocessSuffix) src/sources/iterator.c

$(IntermediateDirectory)/src_sources_memory.c$(ObjectSuffix): src/sources/memory.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/memory.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_memory.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_memory.c$(PreprocessSuffix): src/sources/memory.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_memory.c$(PreprocessSuffix) src/sources/memory.c

$(IntermediateDirectory)/src_sources_network.c$(ObjectSuffix): src/sources/network.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/network.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_network.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_network.c$(PreprocessSuffix): src/sources/network.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_network.c$(PreprocessSuffix) src/sources/network.c

$(IntermediateDirectory)/src_sources_processor.c$(ObjectSuffix): src/sources/processor.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/processor.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_processor.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_processor.c$(PreprocessSuffix): src/sources/processor.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_processor.c$(PreprocessSuffix) src/sources/processor.c

$(IntermediateDirectory)/src_sources_replacer.c$(ObjectSuffix): src/sources/replacer.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/replacer.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_replacer.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_replacer.c$(PreprocessSuffix): src/sources/replacer.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_replacer.c$(PreprocessSuffix) src/sources/replacer.c

$(IntermediateDirectory)/src_sources_script.c$(ObjectSuffix): src/sources/script.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/script.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_script.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_script.c$(PreprocessSuffix): src/sources/script.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_script.c$(PreprocessSuffix) src/sources/script.c

$(IntermediateDirectory)/src_sources_source.c$(ObjectSuffix): src/sources/source.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/source.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_source.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_source.c$(PreprocessSuffix): src/sources/source.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_source.c$(PreprocessSuffix) src/sources/source.c

$(IntermediateDirectory)/src_sources_string.c$(ObjectSuffix): src/sources/string.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/string.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_string.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_string.c$(PreprocessSuffix): src/sources/string.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_string.c$(PreprocessSuffix) src/sources/string.c

$(IntermediateDirectory)/src_sources_timed_action.c$(ObjectSuffix): src/sources/timed_action.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/timed_action.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_timed_action.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_timed_action.c$(PreprocessSuffix): src/sources/timed_action.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_timed_action.c$(PreprocessSuffix) src/sources/timed_action.c

$(IntermediateDirectory)/src_sources_time_s.c$(ObjectSuffix): src/sources/time_s.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/time_s.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_time_s.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_time_s.c$(PreprocessSuffix): src/sources/time_s.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_time_s.c$(PreprocessSuffix) src/sources/time_s.c

$(IntermediateDirectory)/src_sources_webget.c$(ObjectSuffix): src/sources/webget.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/webget.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_webget.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_webget.c$(PreprocessSuffix): src/sources/webget.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_webget.c$(PreprocessSuffix) src/sources/webget.c

$(IntermediateDirectory)/src_sources_wifi.c$(ObjectSuffix): src/sources/wifi.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/wifi.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_wifi.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_wifi.c$(PreprocessSuffix): src/sources/wifi.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_wifi.c$(PreprocessSuffix) src/sources/wifi.c

$(IntermediateDirectory)/src_sources_webget_reborn.c$(ObjectSuffix): src/sources/webget_reborn.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/webget_reborn.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_webget_reborn.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_webget_reborn.c$(PreprocessSuffix): src/sources/webget_reborn.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_webget_reborn.c$(PreprocessSuffix) src/sources/webget_reborn.c

$(IntermediateDirectory)/src_3rdparty_duktape.c$(ObjectSuffix): src/3rdparty/duktape.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/3rdparty/duktape.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_3rdparty_duktape.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_3rdparty_duktape.c$(PreprocessSuffix): src/3rdparty/duktape.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_3rdparty_duktape.c$(PreprocessSuffix) src/3rdparty/duktape.c

$(IntermediateDirectory)/src_3rdparty_unicode_table.c$(ObjectSuffix): src/3rdparty/unicode_table.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/3rdparty/unicode_table.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_3rdparty_unicode_table.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_3rdparty_unicode_table.c$(PreprocessSuffix): src/3rdparty/unicode_table.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_3rdparty_unicode_table.c$(PreprocessSuffix) src/3rdparty/unicode_table.c

$(IntermediateDirectory)/src_crosswin_crosswin.c$(ObjectSuffix): src/crosswin/crosswin.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/crosswin/crosswin.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_crosswin_crosswin.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_crosswin_crosswin.c$(PreprocessSuffix): src/crosswin/crosswin.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_crosswin_crosswin.c$(PreprocessSuffix) src/crosswin/crosswin.c

$(IntermediateDirectory)/src_crosswin_win32.c$(ObjectSuffix): src/crosswin/win32.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/crosswin/win32.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_crosswin_win32.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_crosswin_win32.c$(PreprocessSuffix): src/crosswin/win32.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_crosswin_win32.c$(PreprocessSuffix) src/crosswin/win32.c

$(IntermediateDirectory)/src_crosswin_xlib.c$(ObjectSuffix): src/crosswin/xlib.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/crosswin/xlib.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_crosswin_xlib.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_crosswin_xlib.c$(PreprocessSuffix): src/crosswin/xlib.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_crosswin_xlib.c$(PreprocessSuffix) src/crosswin/xlib.c

$(IntermediateDirectory)/src_image_image_cache.c$(ObjectSuffix): src/image/image_cache.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/image/image_cache.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_image_image_cache.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_image_image_cache.c$(PreprocessSuffix): src/image/image_cache.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_image_image_cache.c$(PreprocessSuffix) src/image/image_cache.c

$(IntermediateDirectory)/src_image_image_cache_bmp.c$(ObjectSuffix): src/image/image_cache_bmp.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/image/image_cache_bmp.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_image_image_cache_bmp.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_image_image_cache_bmp.c$(PreprocessSuffix): src/image/image_cache_bmp.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_image_image_cache_bmp.c$(PreprocessSuffix) src/image/image_cache_bmp.c

$(IntermediateDirectory)/src_image_image_cache_jpeg.c$(ObjectSuffix): src/image/image_cache_jpeg.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/image/image_cache_jpeg.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_image_image_cache_jpeg.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_image_image_cache_jpeg.c$(PreprocessSuffix): src/image/image_cache_jpeg.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_image_image_cache_jpeg.c$(PreprocessSuffix) src/image/image_cache_jpeg.c

$(IntermediateDirectory)/src_image_image_cache_png.c$(ObjectSuffix): src/image/image_cache_png.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/image/image_cache_png.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_image_image_cache_png.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_image_image_cache_png.c$(PreprocessSuffix): src/image/image_cache_png.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_image_image_cache_png.c$(PreprocessSuffix) src/image/image_cache_png.c

$(IntermediateDirectory)/src_image_image_cache_svg.c$(ObjectSuffix): src/image/image_cache_svg.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/image/image_cache_svg.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_image_image_cache_svg.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_image_image_cache_svg.c$(PreprocessSuffix): src/image/image_cache_svg.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_image_image_cache_svg.c$(PreprocessSuffix) src/image/image_cache_svg.c

$(IntermediateDirectory)/src_objects_arc.c$(ObjectSuffix): src/objects/arc.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/arc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_arc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_arc.c$(PreprocessSuffix): src/objects/arc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_arc.c$(PreprocessSuffix) src/objects/arc.c

$(IntermediateDirectory)/src_objects_bar.c$(ObjectSuffix): src/objects/bar.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/bar.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_bar.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_bar.c$(PreprocessSuffix): src/objects/bar.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_bar.c$(PreprocessSuffix) src/objects/bar.c

$(IntermediateDirectory)/src_objects_button.c$(ObjectSuffix): src/objects/button.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/button.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_button.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_button.c$(PreprocessSuffix): src/objects/button.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_button.c$(PreprocessSuffix) src/objects/button.c

$(IntermediateDirectory)/src_objects_histogram.c$(ObjectSuffix): src/objects/histogram.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/histogram.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_histogram.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_histogram.c$(PreprocessSuffix): src/objects/histogram.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_histogram.c$(PreprocessSuffix) src/objects/histogram.c

$(IntermediateDirectory)/src_objects_image.c$(ObjectSuffix): src/objects/image.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/image.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_image.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_image.c$(PreprocessSuffix): src/objects/image.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_image.c$(PreprocessSuffix) src/objects/image.c

$(IntermediateDirectory)/src_objects_line.c$(ObjectSuffix): src/objects/line.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/line.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_line.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_line.c$(PreprocessSuffix): src/objects/line.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_line.c$(PreprocessSuffix) src/objects/line.c

$(IntermediateDirectory)/src_objects_object.c$(ObjectSuffix): src/objects/object.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/object.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_object.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_object.c$(PreprocessSuffix): src/objects/object.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_object.c$(PreprocessSuffix) src/objects/object.c

$(IntermediateDirectory)/src_objects_spinner.c$(ObjectSuffix): src/objects/spinner.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/spinner.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_spinner.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_spinner.c$(PreprocessSuffix): src/objects/spinner.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_spinner.c$(PreprocessSuffix) src/objects/spinner.c

$(IntermediateDirectory)/src_objects_string.c$(ObjectSuffix): src/objects/string.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/objects/string.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_objects_string.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_objects_string.c$(PreprocessSuffix): src/objects/string.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_objects_string.c$(PreprocessSuffix) src/objects/string.c

$(IntermediateDirectory)/src_sources_internal__diag_show.c$(ObjectSuffix): src/sources/internal/_diag_show.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/internal/_diag_show.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_internal__diag_show.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_internal__diag_show.c$(PreprocessSuffix): src/sources/internal/_diag_show.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_internal__diag_show.c$(PreprocessSuffix) src/sources/internal/_diag_show.c

$(IntermediateDirectory)/src_sources_internal__surface_collector.c$(ObjectSuffix): src/sources/internal/_surface_collector.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/internal/_surface_collector.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_internal__surface_collector.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_internal__surface_collector.c$(PreprocessSuffix): src/sources/internal/_surface_collector.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_internal__surface_collector.c$(PreprocessSuffix) src/sources/internal/_surface_collector.c

$(IntermediateDirectory)/src_sources_internal__surface_info.c$(ObjectSuffix): src/sources/internal/_surface_info.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/internal/_surface_info.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_internal__surface_info.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_internal__surface_info.c$(PreprocessSuffix): src/sources/internal/_surface_info.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_internal__surface_info.c$(PreprocessSuffix) src/sources/internal/_surface_info.c

$(IntermediateDirectory)/src_sources_internal__surface_lister.c$(ObjectSuffix): src/sources/internal/_surface_lister.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/internal/_surface_lister.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_internal__surface_lister.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_internal__surface_lister.c$(PreprocessSuffix): src/sources/internal/_surface_lister.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_internal__surface_lister.c$(PreprocessSuffix) src/sources/internal/_surface_lister.c

$(IntermediateDirectory)/src_sources_script_js.c$(ObjectSuffix): src/sources/script/js.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/script/js.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_script_js.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_script_js.c$(PreprocessSuffix): src/sources/script/js.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_script_js.c$(PreprocessSuffix) src/sources/script/js.c

$(IntermediateDirectory)/src_sources_script_lua.c$(ObjectSuffix): src/sources/script/lua.c 
	$(CC) $(SourceSwitch) "D:/Semper_Workspace/Semper/src/sources/script/lua.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sources_script_lua.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sources_script_lua.c$(PreprocessSuffix): src/sources/script/lua.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sources_script_lua.c$(PreprocessSuffix) src/sources/script/lua.c

##
## Clean
##
clean:
	$(RM) -r $(WorkspacePath)/build/$(ProjectName)/


