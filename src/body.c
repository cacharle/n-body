#include "body.h"

void
body_init_random_uniform(struct body *body)
{
    body->x = frand();
    body->y = frand();
    // body->mass = frand() + 0.3;
    body->mass = 0.1f;
    body->velocity_x = 0.0;  // (frand() - 0.5) / 10000;
    body->velocity_y = 0.0;  // (frand() - 0.5) / 10000;
}

void
body_init_random_in_unit_circle(struct body *body)
{
    do
    {
        body_init_random_uniform(body);
        body->x = body->x * 2.0f - 1.0f;
        body->y = body->y * 2.0f - 1.0f;
    } while (sqrtf(body->x * body->x + body->y * body->y) > 0.5f);
    body->x += 0.5f;
    body->y += 0.5f;
}

// static const float gravity = 0.0005;

void
body_gravitational_force(const struct body *b1,
                         const struct body *b2,
                         const float        gravity,
                         float             *force_x,
                         float             *force_y)
{
    *force_x = 0.0;
    *force_y = 0.0;
    if (fabsf(b1->x - b2->x) < 0.01f || fabsf(b1->y - b2->y) < 0.01f)
        return;
    float distance_x = b1->x - b2->x;
    float distance_y = b1->y - b2->y;
    float distance_square = distance_x * distance_x + distance_y * distance_y;
    float force = (b1->mass * b2->mass * gravity) /
                  distance_square;  // maybe we can remove the `b1->mass *` because we end up
                                    // dividing by it at the end

    float dx = b1->x - b2->x;
    float dy = b1->y - b2->y;
    float magnitude_inverse = 1.0 / sqrtf(dx * dx + dy * dy);
    dx *= magnitude_inverse;
    dy *= magnitude_inverse;
    dx *= force;
    dy *= force;
    if (!isnan(dx) && !isnan(dy))
    {
        *force_x = dx;
        *force_y = dy;
    }
}

#include <immintrin.h>

void
body_gravitational_force_avx2(const struct body *dest_body,
                              const struct body  bodies[8],
                              const float        gravity,
                              float             *force_x,
                              float             *force_y)
{
    *force_x = 0.0;
    *force_y = 0.0;
    __m256 m_gravity = _mm256_set1_ps(gravity);

    __m256 bodies_x = _mm256_setr_ps(bodies[0].x,
                                     bodies[1].x,
                                     bodies[2].x,
                                     bodies[3].x,
                                     bodies[4].x,
                                     bodies[5].x,
                                     bodies[6].x,
                                     bodies[7].x);
    __m256 bodies_y = _mm256_setr_ps(bodies[0].y,
                                     bodies[1].y,
                                     bodies[2].y,
                                     bodies[3].y,
                                     bodies[4].y,
                                     bodies[5].y,
                                     bodies[6].y,
                                     bodies[7].y);
    __m256 bodies_mass = _mm256_setr_ps(bodies[0].mass,
                                        bodies[1].mass,
                                        bodies[2].mass,
                                        bodies[3].mass,
                                        bodies[4].mass,
                                        bodies[5].mass,
                                        bodies[6].mass,
                                        bodies[7].mass);

    __m256 dest_x = _mm256_set1_ps(dest_body->x);
    __m256 dest_y = _mm256_set1_ps(dest_body->y);
    __m256 dest_mass = _mm256_set1_ps(dest_body->mass);

    __m256 distance_x = _mm256_sub_ps(bodies_x, dest_x);
    __m256 distance_y = _mm256_sub_ps(bodies_y, dest_y);
    __m256 distance_square =
        _mm256_add_ps(_mm256_mul_ps(distance_x, distance_x), _mm256_mul_ps(distance_y, distance_y));
    __m256 force = _mm256_div_ps(_mm256_mul_ps(_mm256_mul_ps(dest_mass, bodies_mass), m_gravity),
                                 distance_square);

    __m256 dx = _mm256_sub_ps(dest_x, bodies_x);
    __m256 dy = _mm256_sub_ps(dest_y, bodies_y);

    __m256 magnitude_inverse =
        _mm256_rsqrt_ps(_mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)));

    dx = _mm256_mul_ps(dx, magnitude_inverse);
    dy = _mm256_mul_ps(dy, magnitude_inverse);
    dx = _mm256_mul_ps(dx, force);
    dy = _mm256_mul_ps(dy, force);

    float dxs[8] = {0.0};
    float dys[8] = {0.0};
    _mm256_store_ps(dxs, dx);
    _mm256_store_ps(dys, dy);
    for (int i = 0; i < 8; i++)
    {
        if (isnan(dxs[i]) || isinf(dxs[i]))
            dxs[i] = 0.0;
        if (isnan(dys[i]) || isinf(dxs[i]))
            dys[i] = 0.0;
    }
    *force_x = dxs[0] + dxs[1] + dxs[2] + dxs[3] + dxs[4] + dxs[5] + dxs[6] + dxs[7];
    *force_y = dys[0] + dys[1] + dys[2] + dys[3] + dys[4] + dys[5] + dys[6] + dys[7];
}
