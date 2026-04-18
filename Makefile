# Minimal Makefile for the UMSI engine on Linux.
#
# No cmake, no shared library, no rpath — just g++ compiling every solver
# source plus the UMSI front-end into one statically-linked binary.
#
#   make          # build ./minedotcpp_umsi
#   make run      # build and launch interactively
#   make clean    # delete the binary
#
# Not a replacement for the cmake build: this one skips OpenCL, skips
# benchmarking/testcli, and doesn't build the shared library that the
# .NET GUI links against over P/Invoke. It exists purely to make the
# backend trivial to build and iterate on in a Linux shell.

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -pthread
INCLUDES  = -Isrc/minedotcpp -Iexternal/ctpl -Iexternal/sparsehash

# Pull in everything under src/minedotcpp/ (the solver library) plus the
# UMSI entry point. `find` keeps this in sync automatically when new
# source files are added.
SRC := $(shell find src/minedotcpp -name '*.cpp') src/minedotcpp_umsi/main.cpp

BIN  = minedotcpp_umsi

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: run clean
