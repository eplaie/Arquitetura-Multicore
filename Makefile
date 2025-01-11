CXX      := gcc
CXXFLAGS := -Wall -Wextra -Werror
LDFLAGS  := -lstdc++ -lm
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
EXEC_DIR := $(BUILD)/
TARGET   := executable
INCLUDE  := -Iinclude/
SRC      := $(wildcard src/*.c) $(wildcard src/policies/*.c)


OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))

all: build $(EXEC_DIR)/$(TARGET)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(EXEC_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

.PHONY: all build clean debug release run

build:
	@mkdir -p $(EXEC_DIR)
	@mkdir -p $(OBJ_DIR)/src
	@mkdir -p $(OBJ_DIR)/src/policies

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O3
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(EXEC_DIR)/*

run:
	./$(BUILD)/$(TARGET)