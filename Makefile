SRC = src/main.c src/player.c src/zombie.c src/level1.c 
OUT = ECHOES_OF_ASH

ifeq ($(OS),Windows_NT)
    CC = gcc
    CFLAGS = -Wall -std=c99 -Isrc -IC:/raylib/include
    LDFLAGS = -LC:/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm
    OUT = ECHOES_OF_ASH.exe
    RM = del
else
    CC = clang
    CFLAG = -Wall -std=c99 -Isrc -I/opt/homebrew/include
    LDFLAGS = -L/opt/homebrew/lib -lraylib \
              -framework CoreVideo -framework IOKit -framework Cocoa \
              -framework OpenGL -framework GLUT
    RM      = rm -f
endif

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

run: all
	./$(OUT)

clean:
	$(RM) $(OUT)