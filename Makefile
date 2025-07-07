CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS =

# Compiler executable name
TARGET = mylangc

# Source directories
SRC_DIR = src
UTIL_DIR = $(SRC_DIR)/util
CORE_DIR = $(SRC_DIR)/core
# Add other directories like frontend, backend, etc. as they are created

# Find all .c files in source directories
CORE_C_FILES = $(wildcard $(CORE_DIR)/*.c)
UTIL_C_FILES = $(wildcard $(UTIL_DIR)/*.c)
# Include all .c files from src/ directly as well (like main.c)
SRC_C_FILES = $(wildcard $(SRC_DIR)/*.c)

# Consolidate all unique .c files
# $(sort ...) removes duplicates if subdirectories overlap (e.g. src/core is inside src/)
# Ensure no *.c files are directly in src/ if they are also in src/core or src/util to avoid issues,
# or structure includes carefully. For now, main.c is in src/, others in subdirs.
CORE_C_FILES_NODUP = $(filter-out $(SRC_C_FILES), $(CORE_C_FILES))
UTIL_C_FILES_NODUP = $(filter-out $(SRC_C_FILES), $(UTIL_C_FILES))

ALL_C_FILES = $(sort $(SRC_C_FILES) $(CORE_C_FILES_NODUP) $(UTIL_C_FILES_NODUP))


# Generate object file names from all .c files
OBJS = $(patsubst %.c, %.o, $(ALL_C_FILES))

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Generic rule to compile .c files into .o files
# This will apply to main.c as well as other .c files in src/, src/core/, src/util/
# Ensure -I$(SRC_DIR) allows includes like "core/lexer.h" or "util/dynamic_array.h"
# from any .c file in the src tree.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(SRC_DIR)

# Clean target
clean:
	rm -f $(TARGET) $(OBJS)

# Phony targets
.PHONY: all clean

# Initial directory creation message
# This is a comment, actual directory creation will be done via separate tool calls if needed.
# For now, we assume `src/`, `src/util/`, `src/core/` will be created when files are added to them.

# Example usage:
# make
# make clean
