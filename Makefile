# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = 
TARGET = jogo

# Source files (adjust if multiple .c files exist)
SRC = main.c

# Detect OS
UNAME_S := $(shell uname -s)

# Debug vs. Release (run 'make DEBUG=1' for debug build)
ifeq ($(DEBUG),1)
    CFLAGS += -g -O0
else
    CFLAGS += -O2
endif

# Raylib configuration
ifeq ($(shell pkg-config --exists raylib && echo 1),1)
    # Use pkg-config if available (recommended)
    CFLAGS += $(shell pkg-config --cflags raylib)
    LDFLAGS += $(shell pkg-config --libs raylib)
else
    # Platform-specific fallbacks
    ifeq ($(UNAME_S),Darwin)
        # macOS (Homebrew default paths)
        CFLAGS += -I/opt/homebrew/include
        LDFLAGS += -L/opt/homebrew/lib -lraylib
        LDFLAGS += -framework OpenGL -framework Cocoa -framework IOKit
        LDFLAGS += -framework CoreAudio -framework CoreVideo
    else
        # Linux fallback
        # Add local include/ if it exists (optional)
        ifneq ($(wildcard include/raylib.h),)
            CFLAGS += -Iinclude
        endif
        LDFLAGS += -lraylib -lm -ldl -lpthread -lGL -lrt -lX11
    endif
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)