#include "body.h"

static double
frand()
{
    return (double)rand() / (double)RAND_MAX;
}

void
body_init_random(struct body *body)
{
    body->x = frand();
    body->y = frand();
    body->mass = frand() + 0.3;
    body->velocity_x = (frand() - 0.5) / 1000;
    body->velocity_y = (frand() - 0.5) / 1000;
}

static const double gravity = .0000002;

void
body_gravitational_force(const struct body b1,
                         const struct body b2,
                         double           *force_x,
                         double           *force_y)
{
    *force_x = 0.0;
    *force_y = 0.0;
    if (fabs(b1.x - b2.x) < 0.03 || fabs(b1.y - b2.y) < 0.03)
        return;
    double distance_x = b1.x - b2.x;
    double distance_y = b1.y - b2.y;
    double distance_square = distance_x * distance_x + distance_y * distance_y;
    double force = (b1.mass * b2.mass * gravity) / distance_square;

    double dx = b1.x - b2.x;
    double dy = b1.y - b2.y;
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
