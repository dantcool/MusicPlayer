CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c99 -g -Iinclude
LDFLAGS := -lncurses -ltinfo

# Directory structure
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
INCLUDE_DIR := include

# Target and sources
TARGET := $(BIN_DIR)/musicplayer
SRC := $(SRC_DIR)/main.c $(SRC_DIR)/local.c $(SRC_DIR)/spotify.c $(SRC_DIR)/visualizer.c
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

# Build rules
$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

all: $(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/musicplayer

run: $(TARGET)
	$(TARGET)

rebuild: clean all

.PHONY: all clean run rebuild

