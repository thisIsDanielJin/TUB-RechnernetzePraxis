# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.30.5/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.30.5/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build"

# Include any dependencies generated for this target.
include CMakeFiles/zmq_distributor.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/zmq_distributor.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/zmq_distributor.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/zmq_distributor.dir/flags.make

CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o: CMakeFiles/zmq_distributor.dir/flags.make
CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o: /Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze\ und\ Verteilte\ Systeme/Praxis/Praxis\ 3\ Aufgabe-20250130/praxis3/zmq_distributor.c
CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o: CMakeFiles/zmq_distributor.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir="/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o -MF CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o.d -o CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o -c "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/zmq_distributor.c"

CMakeFiles/zmq_distributor.dir/zmq_distributor.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/zmq_distributor.dir/zmq_distributor.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/zmq_distributor.c" > CMakeFiles/zmq_distributor.dir/zmq_distributor.c.i

CMakeFiles/zmq_distributor.dir/zmq_distributor.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/zmq_distributor.dir/zmq_distributor.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/zmq_distributor.c" -o CMakeFiles/zmq_distributor.dir/zmq_distributor.c.s

# Object files for target zmq_distributor
zmq_distributor_OBJECTS = \
"CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o"

# External object files for target zmq_distributor
zmq_distributor_EXTERNAL_OBJECTS =

zmq_distributor: CMakeFiles/zmq_distributor.dir/zmq_distributor.c.o
zmq_distributor: CMakeFiles/zmq_distributor.dir/build.make
zmq_distributor: CMakeFiles/zmq_distributor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir="/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable zmq_distributor"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/zmq_distributor.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/zmq_distributor.dir/build: zmq_distributor
.PHONY : CMakeFiles/zmq_distributor.dir/build

CMakeFiles/zmq_distributor.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/zmq_distributor.dir/cmake_clean.cmake
.PHONY : CMakeFiles/zmq_distributor.dir/clean

CMakeFiles/zmq_distributor.dir/depend:
	cd "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3" "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3" "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build" "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build" "/Users/danielwodke/Dropbox/Uni/Semester_9/Rechnernetze und Verteilte Systeme/Praxis/Praxis 3 Aufgabe-20250130/praxis3/build/CMakeFiles/zmq_distributor.dir/DependInfo.cmake" "--color=$(COLOR)"
.PHONY : CMakeFiles/zmq_distributor.dir/depend

