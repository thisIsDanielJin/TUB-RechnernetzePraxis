# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build"

# Include any dependencies generated for this target.
include CMakeFiles/zmq_worker.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/zmq_worker.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/zmq_worker.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/zmq_worker.dir/flags.make

CMakeFiles/zmq_worker.dir/zmq_worker.c.o: CMakeFiles/zmq_worker.dir/flags.make
CMakeFiles/zmq_worker.dir/zmq_worker.c.o: ../zmq_worker.c
CMakeFiles/zmq_worker.dir/zmq_worker.c.o: CMakeFiles/zmq_worker.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/zmq_worker.dir/zmq_worker.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/zmq_worker.dir/zmq_worker.c.o -MF CMakeFiles/zmq_worker.dir/zmq_worker.c.o.d -o CMakeFiles/zmq_worker.dir/zmq_worker.c.o -c "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/zmq_worker.c"

CMakeFiles/zmq_worker.dir/zmq_worker.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zmq_worker.dir/zmq_worker.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/zmq_worker.c" > CMakeFiles/zmq_worker.dir/zmq_worker.c.i

CMakeFiles/zmq_worker.dir/zmq_worker.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zmq_worker.dir/zmq_worker.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/zmq_worker.c" -o CMakeFiles/zmq_worker.dir/zmq_worker.c.s

# Object files for target zmq_worker
zmq_worker_OBJECTS = \
"CMakeFiles/zmq_worker.dir/zmq_worker.c.o"

# External object files for target zmq_worker
zmq_worker_EXTERNAL_OBJECTS =

zmq_worker: CMakeFiles/zmq_worker.dir/zmq_worker.c.o
zmq_worker: CMakeFiles/zmq_worker.dir/build.make
zmq_worker: CMakeFiles/zmq_worker.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable zmq_worker"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/zmq_worker.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/zmq_worker.dir/build: zmq_worker
.PHONY : CMakeFiles/zmq_worker.dir/build

CMakeFiles/zmq_worker.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/zmq_worker.dir/cmake_clean.cmake
.PHONY : CMakeFiles/zmq_worker.dir/clean

CMakeFiles/zmq_worker.dir/depend:
	cd "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3" "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3" "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build" "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build" "/home/daniel-jin/Desktop/TUB-RechnernetzePraxis/Praxis 3 Aufgabe-20250130/praxis3/build/CMakeFiles/zmq_worker.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/zmq_worker.dir/depend

