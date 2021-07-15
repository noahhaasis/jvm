#!/usr/bin/sh
gcc -ggdb -o main main.c parse.c runtime.c buffer.h  -Wall -std=c11 -D DEBUG
