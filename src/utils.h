#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
die(char *format, ...);
void *
xmalloc(size_t size);
double
frand();


#endif
