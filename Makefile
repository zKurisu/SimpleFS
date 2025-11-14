CC = gcc
CFLAGS = -Wall

BUILD_DIR = build
TARGET = simplefs
TARGET_PATH = $(BUILD_DIR)/$(TARGET)

SRC_DIR = src
SRC = main.c
SRC_PATH = $(SRC_DIR)/$(SRC)

build: $(TARGET_PATH)

$(TARGET_PATH): $(SRC_PATH)
	mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

run:
	@./$(TARGET_PATH)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: build
