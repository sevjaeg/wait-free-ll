# project name (generate executable with this name)
TARGET   = wfll

GCC       = g++
# compiling flags here
CFLAGS   =  -Wall -fopenmp -O3

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

all: dirs $(BINDIR)/$(TARGET)
	@echo "Target ${TARGET} built sucessfully!"
	
dirs:
	mkdir -p $(OBJDIR) $(BINDIR)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"


$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(GCC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"


clean:
	@$(rm) $(OBJECTS)$(BINDIR)/$(TARGET): $(OBJECTS)

.PHONY: clean dirs all
