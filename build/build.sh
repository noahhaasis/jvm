#!/usr/bin/sh
gcc -ggdb -o ./bin/main src/main.c src/parse.c src/runtime.c src/runtime.h src/class_loader.c src/class_loader.h src/buffer.h  src/hashmap.h src/hashmap.c src/common.h src/class_file.h -Wall -std=c11 -D DEBUG
# g++ -ggdb -o ./bin/main main.c parse.c runtime.c class_loader.c buffer.h  hashmap.h hashmap.c common.h -Wall -fpermissive -D DEBUG
