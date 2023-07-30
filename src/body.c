#include "body.h"

static double
frand()
{
    return (double)rand() / (double)RAND_MAX;
}

void
body_init_random_uniform(struct body *body)
{
    body->x = frand();
    body->y = frand();
    // body->mass = frand() + 0.3;
    body->mass = 0.1;
    body->velocity_x = 0.0; //(frand() - 0.5) / 10000;
    body->velocity_y = 0.0; //(frand() - 0.5) / 10000;
}

void
body_init_random_in_unit_circle(struct body *body)
{
    do
    {
        body_init_random_uniform(body);
        body->x = body->x * 2 - 1;
        body->y = body->y * 2 - 1;
    }
    while (sqrt(body->x * body->x + body->y * body->y) > 0.5);
    body->x += 0.5;
    body->y += 0.5;
}

static const double gravity = 0.0005;

void
body_gravitational_force(const struct body *b1,
                         const struct body *b2,
                         double            *force_x,
                         double            *force_y)
{
    *force_x = 0.0;
    *force_y = 0.0;
    if (fabs(b1->x - b2->x) < 0.01 || fabs(b1->y - b2->y) < 0.01)
        return;
    double distance_x = b1->x - b2->x;
    double distance_y = b1->y - b2->y;
    double distance_square = distance_x * distance_x + distance_y * distance_y;
    double force = (b1->mass * b2->mass * gravity) /
                   distance_square;  // maybe we can remove the `b1->mass *` because we end up
                                     // dividing by it at the end

    double dx = b1->x - b2->x;
    double dy = b1->y - b2->y;
    double magnitude = sqrt(dx * dx + dy * dy);
    dx /= magnitude;
    dy /= magnitude;
    dx *= force;
    dy *= force;
    if (!isnan(dx) && !isnan(dy))
    {
        *force_x = dx;
        *force_y = dy;
    }
}
