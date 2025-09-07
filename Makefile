# Makefile for mmn14 server on Windows with Visual Studio 

# Compiler and Linker
# using LLVM clang from Visual Studio Build Tools
CXX = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\x64\bin\clang++.exe"
LINK = $(CXX)

# Compiler flags
# -std:c++17 - Use C++17 standard
# -I - Add include directory
# This is where I have Boost installed
BOOST_PATH = C:\local\boost_1_89_0
CXXFLAGS = -std=c++17 -D_WIN32_WINNT=0x0A00 -I"$(BOOST_PATH)"

# Linker flags
# -o - Output file name
LINKFLAGS = -o server.exe

# Source files and Object files
SRCS = main.cpp RequestHandler.cpp
OBJS = main.o RequestHandler.o

# Default target
all: server.exe

# Rule to build the executable
server.exe: $(OBJS)
    $(LINK) $(OBJS) $(LINKFLAGS)

# Rule to compile .cpp files to .obj files
.cpp.o:
    $(CXX) -c $(CXXFLAGS) $<

# Clean target
clean:
    del *.o *.exe *.ilk *.pdb
