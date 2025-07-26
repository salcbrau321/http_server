CC := gcc
CFLAGS := -Wall -Wextra -g -Iinclude -fsanitize=address
LDFLAGS := -fsanitize=address

SRC_DIR := src
TEST_DIR := tests
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
	$(CC) $(CFLAGS) -o $(DEMO_BIN) $(DEMO_SRC) $(LIB) $(LDFLAGS)

$(BUILD_DIR)/%.test.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(TEST_DIR) -c $< -o $@

$(TEST_BIN): $(LIB) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) $(LIB) -lcriterion $(LDFLAGS)

test: $(TEST_BIN)
	@echo "=== Running HTTP Server unit tests ==="
	@./$(TEST_BIN)

# Clean up
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

