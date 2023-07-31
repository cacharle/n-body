#ifndef BODY_H
#define BODY_H

#include "utils.h"
#include <immintrin.h>
#include <math.h>
#include <stdlib.h>

struct body
{
    float mass;
    float x;
    float y;
    float velocity_x;
    float velocity_y;
};

void
body_init_random_uniform(struct body *body);
void
body_init_random_circle(struct body *body);
void
body_init_random_thorus(struct body *body);
void
body_gravitational_force(const struct body *b1,
                         const struct body *b2,
                         const float        gravity,
                         float             *force_x,
                         float             *force_y);

void
body_gravitational_force_avx2(const struct body *dest_body,
                              const struct body  bodies[8],
                              const float        gravity,
                              float             *force_x,
                              float             *force_y);

#endif
