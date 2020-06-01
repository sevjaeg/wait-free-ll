# project name (generate executable with this name)
TARGET   = wfll

GCC       = g++
# compiling flags here
CFLAGS   =  -Wall -fopenmp -O0

LINKER   = g++ -fopenmp
# linking flags here
LFLAGS   = -Wall -lm 

# change these to proper directories where each file should be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
rm       = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"


$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(GCC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"


.PHONY: clean
clean:
	@$(rm) $(OBJECTS)$(BINDIR)/$(TARGET): $(OBJECTS)
