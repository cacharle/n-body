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

// from: https://en.wikipedia.org/wiki/Fast_inverse_square_root
// TODO: watch the famous YT video on it
float
rsqrt(float number)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    // y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
    return y;
}
