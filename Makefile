CC := gcc
TEST_DEBUG ?= 1
ifeq ($(TEST_DEBUG),1)
DBGFLAGS := -g -O0 -fno-inline
else
DBGFLAGS :=
endif
CFLAGS := -Wall -Wextra -Iinclude $(DBGFLAGS)

TEST_DIR := tests
TEST_CFLAGS := -Wall -Wextra -Iinclude -I$(TEST_DIR) $(DBGFLAGS)

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
LIB := $(BUILD_DIR)/libhttpserver.a

DEMO_SRC := examples/hello_app.c
DEMO_BIN := $(BIN_DIR)/hello_server

TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.test.o,$(TEST_SRCS))
TEST_BIN := $(BUILD_DIR)/http_server_tests

.PHONY: all demo test clean

all: $(LIB)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	ar rcs $@ $^

demo: all | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(DEMO_BIN) $(DEMO_SRC) $(LIB)

$(BUILD_DIR)/%.test.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_BIN): $(LIB) $(TEST_OBJS)
	$(CC) $(TEST_CFLAGS) -o $@ $(TEST_OBJS) $(LIB) -lcriterion

test: $(TEST_BIN)
	@echo "=== Running HTTP Server unit tests ==="
	@./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

