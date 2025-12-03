# ============================================
# Dir and file config
# ============================================
BUILD_DIR := build
SRC_DIR   := src
LIB_DIR   := $(SRC_DIR)/lib
UTILS_DIR := $(SRC_DIR)/utils
TEST_DIR  := test

# ============================================
# Compiler config
# ============================================
CC      := gcc
CFLAGS  := -g -Wall -I$(LIB_DIR)
LDFLAGS := -lpthread

# ============================================
# Files catch
# ============================================
LIB_SRCS := $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS := $(patsubst $(LIB_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))

# ============================================
# Target defs
# ============================================
TARGET_SRC  := $(SRC_DIR)/main.c
TARGET 		:= $(BUILD_DIR)/simplefs
TEST_TARGET := $(BUILD_DIR)/test

# ============================================
# Command list
# ============================================
.PHONY: all build run test clean

# Prevent Make from auto-deleting intermediate files
.PRECIOUS: $(BUILD_DIR)/my_% $(BUILD_DIR)/test_%

all: build

build: $(TARGET)

$(TARGET): $(TARGET_SRC) $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

# ============================================
# Utils
# ============================================
cmd-%: $(BUILD_DIR)/my_%
	./$<

$(BUILD_DIR)/my_%: $(UTILS_DIR)/%.c $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CLAGS) $^ -o $@ $(LDFLAGS)

# ============================================
# Test
# ============================================

test-%: $(BUILD_DIR)/test_%
	./$<

$(BUILD_DIR)/test_%: $(TEST_DIR)/%.c $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

print-%:
	@echo '$*=$($*)'
	@echo '  origin = $(origin $*)'
	@echo '  flavor = $(flavor $*)'
	@echo '  value  = $($*)'
