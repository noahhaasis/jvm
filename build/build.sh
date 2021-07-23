#!/usr/bin/sh
gcc -ggdb -o ./bin/main main.c parse.c runtime.c class_loader.c buffer.h  hashmap.h hashmap.c common.h -Wall -std=c11 -D DEBUG
