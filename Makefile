# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -lm

# Directories
SRC_DIR = src
OUT_DIR = out

# Files
SRCS = $(SRC_DIR)/main.c
TARGET = $(OUT_DIR)/shell

# Program binary
all: $(TARGET)

# Object linking
$(TARGET): $(SRCS)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean-up script
clean:
	rm -rf $(OUT_DIR)
