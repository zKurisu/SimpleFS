BUILD_DIR = build
TARGET = simplefs
TARGET_PATH = $(BUILD_DIR)/$(TARGET)

SRC_DIR = src
SRC = main.c
SRC_PATH = $(SRC_DIR)/$(SRC)

TEST_DIR = test
TEST_SRC = check_block.c
TEST_SRC_PATH = $(TEST_DIR)/$(TEST_SRC)
TEST_TARGET = test
TEST_TARGET_PATH = $(BUILD_DIR)/$(TEST_TARGET)

LIB_DIR = $(SRC_DIR)/lib
UTILS_DIR = $(SRC_DIR)/utils
CC = gcc
CFLAGS = -g -Wall -I$(LIB_DIR) -I$(UTILS_DIR)

build: $(TARGET_PATH)

$(TARGET_PATH): $(SRC_PATH)
	mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

run:
	./$(TARGET_PATH)

test: build $(TEST_TARGET_PATH)
	./$(TEST_TARGET_PATH)

$(TEST_TARGET_PATH): $(TEST_SRC_PATH) $(BUILD_DIR)/disk.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/block.o $(BUILD_DIR)/inode.o $(BUILD_DIR)/fs.o $(BUILD_DIR)/helper.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/disk.o: $(LIB_DIR)/disk.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bitmap.o: $(LIB_DIR)/bitmap.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/block.o: $(LIB_DIR)/block.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/inode.o: $(LIB_DIR)/inode.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fs.o: $(LIB_DIR)/fs.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/helper.o: $(UTILS_DIR)/helper.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: build run test clean
