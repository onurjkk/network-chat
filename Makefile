ifeq ($(OS),Windows_NT)
    EXE = .exe
    RM = del /Q
    CC = gcc
    CFLAGS = -Wall -Wextra -g
    LDFLAGS = -lws2_32
else
    EXE =
    RM = rm -f
    CC = gcc
    CFLAGS = -Wall -Wextra -g -pthread
    LDFLAGS =
endif

SRC_DIR = src
BUILD_DIR = build
TARGETS = $(BUILD_DIR)/server$(EXE) $(BUILD_DIR)/client$(EXE)

all: $(TARGETS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/server$(EXE): $(SRC_DIR)/server.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/client$(EXE): $(SRC_DIR)/client.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	$(RM) $(BUILD_DIR)/server$(EXE) $(BUILD_DIR)/client$(EXE)

.PHONY: all clean