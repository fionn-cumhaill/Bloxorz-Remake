all: sample2D

sample2D: bloxorz.cpp glad.c
	g++ -o Bloxorz bloxorz.cpp glad.c -lao -lmpg123 -lm -lGL -lglfw -ldl

debug := CFLAGS= -g

clean:
	rm Bloxorz
