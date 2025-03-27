# Compiler
CXX = g++
CXXFLAGS = -w -std=c++11

# Directories
SRC_DIR = .
BIN_DIR = .
OBJ_DIR = .

# Targets
TARGETS = $(BIN_DIR)/forward $(BIN_DIR)/noforward

# Source files
SRC_FORWARD = Processor_F.cpp Decoder_F.cpp
SRC_NOFORWARD = Processor_NF.cpp Decoder_NF.cpp

# Object files
OBJ_FORWARD = $(SRC_FORWARD:.cpp=.o)
OBJ_NOFORWARD = $(SRC_NOFORWARD:.cpp=.o)

# Header files (include all .hpp files)
HEADERS = $(wildcard *.hpp)

# Build all targets
all: $(TARGETS)

# Forwarding processor
$(BIN_DIR)/forward: $(OBJ_FORWARD)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Non-forwarding processor
$(BIN_DIR)/noforward: $(OBJ_NOFORWARD)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ_FORWARD) $(OBJ_NOFORWARD) $(TARGETS)

# Phony targets
.PHONY: all clean
