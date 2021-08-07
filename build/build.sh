#!/usr/bin/sh
g++ -ggdb -o ./bin/main src/main.cpp src/parse.cpp src/runtime.cpp src/runtime.h src/class_loader.cpp src/class_loader.h src/buffer.h  src/common.h src/class_file.h src/string.cpp src/string.h src/HashMap.h -fpermissive -std=gnu++20 -D DEBUG
# g++ -ggdb -o ./bin/main main.cpp parse.cpp runtime.cpp class_loader.cpp buffer.h  hashmap.h hashmap.cpp common.h -Wall -fpermissive -D DEBUG
