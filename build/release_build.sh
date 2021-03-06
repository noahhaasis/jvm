#!/usr/bin/sh
g++ -O3 -o ./bin/main \
  src/main.cpp \
  src/parse.cpp \
  src/runtime.cpp \
  src/runtime.h \
  src/class_loader.cpp \
  src/class_loader.h \
  src/common.h \
  src/class_file.h \
  src/string.cpp \
  src/string.h \
  -fpermissive -std=gnu++20
# -fsanitize=address -g
# g++ -ggdb -o ./bin/main main.cpp parse.cpp runtime.cpp class_loader.cpp buffer.h  hashmap.h hashmap.cpp common.h -Wall -fpermissive -D DEBUG
