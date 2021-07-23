#!/usr/bin/sh
gcc -ggdb -o ./bin/test test.c parse.c runtime.c class_loader.c buffer.h  hashmap.h hashmap.c -Wall -std=c11 -D DEBUG && ./bin/test
