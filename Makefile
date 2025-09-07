# Makefile for mmn14 server on Windows with Visual Studio 
# To be used with nmake

# Use this target to build from a regular command prompt
# It will set up the Visual Studio environment first.
# VCINSTALLDIR should be set in your environment. This is the base folder for Visual Studio build tools. In my environment it is set to:
# C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC
# Example: nmake build
build:
	call "$(VCINSTALLDIR)\Auxiliary\Build\vcvarsall.bat" x64 && $(MAKE) all

# Compiler and Linker
CXX = "cl.exe"
LINK = $(CXX)

# Compiler flags
# /std:c++17 - Use C++17 standard
# /I - Add include directory
# /D - Define a preprocessor macro
# /EHsc - Enable C++ exception handling
CXXFLAGS =  /c /std:c++17 /D "_WIN32_WINNT=0x0A00" /EHsc -I.
EXE=server.exe

# Linker flags
# /Fe - Output file name
LINKFLAGS = /Fe:$(EXE) 

# Source files and Object files
SRCS = RequestHandler.cpp main.cpp 
OBJS = RequestHandler.obj main.obj 

# Default target
all: $(EXE)

# Rule to build the executable
$(EXE): $(OBJS)
    $(LINK) $(OBJS) $(LINKFLAGS)

# Rule to compile .cpp files to .obj files
.cpp.obj:
    $(CXX) $(CXXFLAGS) $<

# Clean target
clean:
    del *.obj *.exe *.ilk *.pdb