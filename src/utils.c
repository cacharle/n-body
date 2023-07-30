#include "utils.h"

void
die(char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "Error: %s: ", strerror(errno));
    vfprintf(stderr, format, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

void *
xmalloc(size_t size)
{
    void *x = malloc(size);
    if (x == NULL)
        die("Invalid malloc");
    return x;
}

double
frand()
{
    return (double)rand() / (double)RAND_MAX;
}

