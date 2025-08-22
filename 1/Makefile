CC = clang
CFLAGS = -g -DDEBUGGER
CFLAGS += -I/opt/homebrew/include -I../../../shared/stb -I../../../other/cimgui
CFLAGS += -DGL_SILENCE_DEPRECATION
CFLAGS += -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
LFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL

CIMGUIRPATH = -Wl,-rpath,/Users/struc/dev/other/cimgui/backend_test/example_glfw_opengl3/build

editor: bin/editor

run: editor
	lldb bin/editor -o run

debug: editor
	lldb bin/editor

clean:
	rm -rf bin
	mkdir bin

.PHONY: editor run debug clean

bin/common.o: $(wildcard src/common/*)
	$(CC) $(CFLAGS) -c src/common/common.c -o $@

bin/editor: src/main.c bin/common.o
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@
