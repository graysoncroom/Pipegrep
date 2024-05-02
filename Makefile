CXX=g++                                # compiler
CXXFLAGS=-std=c++11 -Wall -Wextra -g   # compiler flags
LDFLAGS=-pthread                       # linker flags

# Directories
SRCDIR := src
BUILDDIR := build
BINDIR := bin

HEADERS := $(shell find $(SRCDIR) -type f -name "*.h")
SOURCES := $(shell find $(SRCDIR) -type f -name "*.cpp")
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SOURCES))
EXECUTABLE := $(BINDIR)/pipegrep

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	@rm -rf $(BUILDDIR) $(BINDIR)

.PHONY: all bulid clean

# Notes for future Grayson (and other ppl not so comfy with Make)
# In the $(EXECUTABLE): $(OBJECTS) line, $(EXECUTABLE) is the name of the target
# Since, $(EXECUTABLE) is defined to be $(BINDIR)/mutual and $(BINDIR) is defined
# to be bin, we have that the target name is just bin/mutual in this case.
#
# $@ refers to the target name (e.g. bin/mutual)
# $< refers to the first object file in $(OBJECTS)
# $^ refers to all object files in a space delimited list
#
# The $(EXECUTABLE) target's goal is to build the final product.
# It does this by specifying the compiler, compiler flags, linker flags,
# passes the executable we are trying to generate (target name) to the -o flag,
# and finally compiles all object files into that executable by putting $^ at the
# end.
#
# Finally, the $(BUILDDIR)%.o target defines how we generate each object file
# from each source code file. This is used by the $(EXECUTABLE) target when
# processing it's objects.
