all:
	g++ -o bin/fortune-chess src/*.cpp src/tinycthread.c -I src -O3