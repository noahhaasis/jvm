#ifndef JVM_H
#define JVM_H

#include "common.h"
#include "class_loader.h"

void execute_main(char *filename);

void execute(ClassLoader class_loader, Method *entry_method);

#endif
