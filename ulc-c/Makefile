# ULC C Implementation Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude
LDFLAGS = -llzma

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TARGET = ulc.exe

# Source files
SOURCES = $(SRC_DIR)/ulc_utils.c \
          $(SRC_DIR)/ulc_parser.c \
          $(SRC_DIR)/ulc_compress.c \
          $(SRC_DIR)/ulc_cli.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo Build complete: $(TARGET)

# Clean build artifacts
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	@if exist "$(TARGET)" del /q "$(TARGET)"
	@echo Clean complete

# Rebuild
rebuild: clean all

.PHONY: all clean rebuild
