CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -fsanitize=address
BUILD_DIR = build
BIN_DIR = bin
SRC_DIR = src
OBJ = $(BUILD_DIR)/http_server.o $(BUILD_DIR)/router.o $(BUILD_DIR)/http_request.o $(BUILD_DIR)/http_response.o $(BUILD_DIR)/http_request_parser.o  
LIB = $(BUILD_DIR)/libhttpserver.a
DEMO_BIN = $(BIN_DIR)/hello_server

all: $(LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	ar rcs $@ $^

demo: all | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(DEMO_BIN) examples/hello_app.c $(LIB)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
