# ============================================================
#  ECHOES OF ASH  -  one Makefile, every platform
#
#    make          build the game
#    make run      build it, then play it
#    make clean    delete the built files
#
#  macOS, Windows and Linux. The OS is detected automatically -
#  you do not configure anything.
#
#  NOTE: the indented lines below MUST start with a real TAB,
#  not spaces. If make says "missing separator", that's why.
# ============================================================

SRC = $(wildcard src/*.c)

# ---------- which operating system are we on? ----------
ifeq ($(OS),Windows_NT)
    PLATFORM = WINDOWS
else
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        PLATFORM = MACOS
    else
        PLATFORM = LINUX
    endif
endif

# ---------------- macOS ----------------
ifeq ($(PLATFORM),MACOS)
    CC      = clang
    OUT     = echoes-of-ash
    RUN     = ./$(OUT)
    RM      = rm -f
    # Homebrew is /opt/homebrew on Apple Silicon, /usr/local on Intel.
    # Both are listed; the compiler ignores whichever isn't there.
    CFLAGS  = -Wall -Wextra -std=c99 -Isrc -I/opt/homebrew/include -I/usr/local/include
    LDFLAGS = -L/opt/homebrew/lib -L/usr/local/lib -lraylib -lm \
              -framework Cocoa -framework IOKit -framework OpenGL \
              -framework CoreVideo -framework CoreAudio
endif

# ---------------- Windows ----------------
ifeq ($(PLATFORM),WINDOWS)
    CC      = gcc
    OUT     = echoes-of-ash.exe
    RUN     = $(OUT)
    RM      = del /Q
    # Where you unzipped raylib. Forward slashes, even on Windows.
    # Override without editing this file:   make RAYLIB_PATH=D:/stuff/raylib
    RAYLIB_PATH ?= C:/raylib/raylib
    CFLAGS  = -Wall -Wextra -std=c99 -Isrc -I$(RAYLIB_PATH)/src
    LDFLAGS = -L$(RAYLIB_PATH)/src -lraylib -lopengl32 -lgdi32 -lwinmm -lm
endif

# ---------------- Linux ----------------
ifeq ($(PLATFORM),LINUX)
    CC      = gcc
    OUT     = echoes-of-ash
    RUN     = ./$(OUT)
    RM      = rm -f
    CFLAGS  = -Wall -Wextra -std=c99 -Isrc
    LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11
endif

# ---------------- targets ----------------
all: $(OUT)

$(OUT): $(SRC) src/game.h src/tuning.h src/beams.h
	@echo Building for $(PLATFORM)...
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)
	@echo Done. Play it with:  make run

run: $(OUT)
	$(RUN)

clean:
	-$(RM) echoes-of-ash echoes-of-ash.exe

.PHONY: all run clean