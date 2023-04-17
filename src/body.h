#ifndef BODY_H
#define BODY_H

#include <math.h>
#include <stdlib.h>

struct body
{
    double mass;
    double x;
    double y;
    double velocity_x;
    double velocity_y;
};

void
body_init_random(struct body *body);
void
body_gravitational_force(const struct body b1,
                         const struct body b2,
                         double           *force_x,
                         double           *force_y);

#endif
