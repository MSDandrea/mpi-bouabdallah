# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/matheus/CLionProjects/Naimi-Trehel

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/Naimi_Trehel.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Naimi_Trehel.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Naimi_Trehel.dir/flags.make

CMakeFiles/Naimi_Trehel.dir/main.c.o: CMakeFiles/Naimi_Trehel.dir/flags.make
CMakeFiles/Naimi_Trehel.dir/main.c.o: ../main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/Naimi_Trehel.dir/main.c.o"
	/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Naimi_Trehel.dir/main.c.o   -c /home/matheus/CLionProjects/Naimi-Trehel/main.c

CMakeFiles/Naimi_Trehel.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Naimi_Trehel.dir/main.c.i"
	/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matheus/CLionProjects/Naimi-Trehel/main.c > CMakeFiles/Naimi_Trehel.dir/main.c.i

CMakeFiles/Naimi_Trehel.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Naimi_Trehel.dir/main.c.s"
	/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matheus/CLionProjects/Naimi-Trehel/main.c -o CMakeFiles/Naimi_Trehel.dir/main.c.s

# Object files for target Naimi_Trehel
Naimi_Trehel_OBJECTS = \
"CMakeFiles/Naimi_Trehel.dir/main.c.o"

# External object files for target Naimi_Trehel
Naimi_Trehel_EXTERNAL_OBJECTS =

Naimi_Trehel: CMakeFiles/Naimi_Trehel.dir/main.c.o
Naimi_Trehel: CMakeFiles/Naimi_Trehel.dir/build.make
Naimi_Trehel: /usr/lib/openmpi/libmpi.so
Naimi_Trehel: CMakeFiles/Naimi_Trehel.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable Naimi_Trehel"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Naimi_Trehel.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Naimi_Trehel.dir/build: Naimi_Trehel

.PHONY : CMakeFiles/Naimi_Trehel.dir/build

CMakeFiles/Naimi_Trehel.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Naimi_Trehel.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Naimi_Trehel.dir/clean

CMakeFiles/Naimi_Trehel.dir/depend:
	cd /home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matheus/CLionProjects/Naimi-Trehel /home/matheus/CLionProjects/Naimi-Trehel /home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug /home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug /home/matheus/CLionProjects/Naimi-Trehel/cmake-build-debug/CMakeFiles/Naimi_Trehel.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Naimi_Trehel.dir/depend
