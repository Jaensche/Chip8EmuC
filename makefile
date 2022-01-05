# D:\MySYS2\usr\bin\make.exe

all:
	gcc -Isrc/include -Lsrc/lib -g -o main main.c -lmingw32 -lSDL2main -lSDL2 -mwindows