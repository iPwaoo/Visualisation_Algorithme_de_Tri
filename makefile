# --- Compilation settings ---
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lm -lSDL2_image

# --- Source and build directories ---
SRC = main.c sorting.c visual.c stats.c utils.c
OUT_DIR = out
OBJ = $(patsubst %.c,$(OUT_DIR)/%.o,$(SRC))
TARGET = main

# --- Default target ---
all: $(OUT_DIR) $(TARGET)

# --- Link final executable ---
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# --- Compile each .c into out/ ---
$(OUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Create build folder if missing ---
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# --- Clean build files ---
clean:
	rm -rf $(OUT_DIR) $(TARGET)
