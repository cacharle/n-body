#include "utils.h"

void
die(const char *format, ...)
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

float
frand(void)
{
    return (float)rand() / (float)RAND_MAX;
}

// from: https://en.wikipedia.org/wiki/Fast_inverse_square_root
// TODO: watch the famous YT video on it
float
rsqrt(float number)
{
    union
    {
        float    f;
        uint32_t i;
    } conv = {.f = number};
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f *= 1.5F - (number * 0.5F * conv.f * conv.f);
    return conv.f;
}
