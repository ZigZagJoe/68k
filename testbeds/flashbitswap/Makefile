### ZZJ's Fancypants Makefile for C++
# You should not need to modify this file unless on different OS.

####### VARIABLES #######

# Get directory name, to use as output filename.
BIN = $(notdir $(CURDIR))

# Program names

RM   = rm -f
CPP  = g++
CC   = gcc

# Compiler and linker flags

CXXFLAGS =
LDFLAGS  =
LIBS = -lstdc++

# List of source files

SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)

####### TARGETS ########

all:	$(BIN)

clean:
	$(RM) $(OBJS) ./$(BIN)

zip:	clean all
	./$(BIN) > output.txt
	zip -vr $(BIN).zip . -x $(BIN).zip

# Grab-n-go: cleans, builds, runs output.
run:	clean all
	./$(BIN)

debug:  clean all
	gdb ./$(BIN)

# Open source in default editor, on mac.
open:
	-open $(SRCS) 

$(BIN): $(OBJS)    
	$(CC) $(LDFLAGS) $(LIBS) -o $(BIN) $(OBJS) 

.o:
	$(CPP) $(CXXFLAGS) -c -o $@ $<

