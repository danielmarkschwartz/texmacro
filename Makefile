CFLAGS=-Wall -g -O0 -rdynamic

test: tex
	./tex

tex: tex.c
