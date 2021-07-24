#ifndef JVM_H
#define JVM_H

#include "common.h"
#include "class_file.h"

code_attribute *find_code(ClassFile *class_file, method_info method_info);

method_info find_method(ClassFile *class_file, char *name, u32 name_length);

void execute(ClassFile *class_file, code_attribute method_code);

#endif
