CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude
SRC_DIR = src
OBJ_DIR = build
TARGET = shell

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubmain:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(OBJ_DIR) $(TARGET) .shell_history

.PHONY: all clean