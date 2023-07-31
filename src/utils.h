#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

void
die(char *format, ...);
void *
xmalloc(size_t size);
float
frand();
float
rsqrt(float number);


#endif
