#include "quadtree.h"
#include "body.h"

static bool
in_boundary(struct quadtree *quadtree, struct body body)
{
    return body.x <= quadtree->end_x && body.x >= quadtree->start_x && body.y <= quadtree->end_y &&
           body.y >= quadtree->start_y;
}

static struct quadtree *
quadtree_new_no_bounding_box()
{
    struct quadtree *quadtree = malloc(sizeof(struct quadtree));
    assert(quadtree != NULL);
    memset(quadtree, 0, sizeof *quadtree);
    quadtree->type = QUADTREE_EMPTY;
    return quadtree;
}

struct quadtree *
quadtree_new(struct body *bodies, size_t bodies_count)
{
    struct quadtree *quadtree = quadtree_new_no_bounding_box();
    quadtree->start_x = INFINITY;
    quadtree->start_y = INFINITY;
    quadtree->end_x = -INFINITY;
    quadtree->end_y = -INFINITY;
    for (size_t i = 0; i < bodies_count; i++)
    {
        if (bodies[i].x < quadtree->start_x)
            quadtree->start_x = bodies[i].x;
        if (bodies[i].y < quadtree->start_y)
            quadtree->start_y = bodies[i].y;
        if (bodies[i].x > quadtree->end_x)
            quadtree->end_x = bodies[i].x;
        if (bodies[i].y > quadtree->end_y)
            quadtree->end_y = bodies[i].y;
    }
    return quadtree;
}

void
quadtree_destroy(struct quadtree *quadtree)
{
    if (quadtree->type == QUADTREE_INTERNAL)
    {
        quadtree_destroy(quadtree->internal.nw);
        quadtree_destroy(quadtree->internal.ne);
        quadtree_destroy(quadtree->internal.sw);
        quadtree_destroy(quadtree->internal.se);
    }
    free(quadtree);
}

void
quadtree_insert(struct quadtree *quadtree, struct body body)
{
    struct quadtree *child = NULL;
    if (!in_boundary(quadtree, body))
        return;
    switch (quadtree->type)
    {
    case QUADTREE_EMPTY:
        quadtree->type = QUADTREE_EXTERNAL;
        quadtree->external.bodies[0] = body;
        quadtree->external.bodies_count = 1;
        break;
    case QUADTREE_EXTERNAL:
        if (quadtree->external.bodies_count < QUADTREE_MAX_BODIES_COUNT)
        {
            quadtree->external.bodies[quadtree->external.bodies_count] = body;
            quadtree->external.bodies_count++;
            break;
        }
        quadtree->type = QUADTREE_INTERNAL;
        struct body original_bodies[QUADTREE_MAX_BODIES_COUNT];
        memcpy(original_bodies, quadtree->external.bodies, sizeof original_bodies);
        quadtree->internal.nw = quadtree_new_no_bounding_box();
        quadtree->internal.ne = quadtree_new_no_bounding_box();
        quadtree->internal.sw = quadtree_new_no_bounding_box();
        quadtree->internal.se = quadtree_new_no_bounding_box();
        float mid_x = quadtree->start_x + (quadtree->end_x - quadtree->start_x) / 2.0f;
        float mid_y = quadtree->start_y + (quadtree->end_y - quadtree->start_y) / 2.0f;
        // nw
        quadtree->internal.nw->start_x = quadtree->start_x;
        quadtree->internal.nw->end_x = mid_x;
        quadtree->internal.nw->start_y = quadtree->start_y;
        quadtree->internal.nw->end_y = mid_y;
        // ne
        quadtree->internal.ne->start_x = mid_x;
        quadtree->internal.ne->end_x = quadtree->end_x;
        quadtree->internal.ne->start_y = quadtree->start_y;
        quadtree->internal.ne->end_y = mid_y;
        // sw
        quadtree->internal.sw->start_x = quadtree->start_x;
        quadtree->internal.sw->end_x = mid_x;
        quadtree->internal.sw->start_y = mid_y;
        quadtree->internal.sw->end_y = quadtree->end_y;
        // se
        quadtree->internal.se->start_x = mid_x;
        quadtree->internal.se->end_x = quadtree->end_x;
        quadtree->internal.se->start_y = mid_y;
        quadtree->internal.se->end_y = quadtree->end_y;
        // reinsert the original bodies
        for (size_t i = 0; i < QUADTREE_MAX_BODIES_COUNT; i++)
            quadtree_insert(quadtree, original_bodies[i]);
        quadtree_insert(quadtree, body);  // treated as an internal node now
        break;
    case QUADTREE_INTERNAL:
        if (in_boundary(quadtree->internal.nw, body))
            child = quadtree->internal.nw;
        else if (in_boundary(quadtree->internal.ne, body))
            child = quadtree->internal.ne;
        else if (in_boundary(quadtree->internal.sw, body))
            child = quadtree->internal.sw;
        else if (in_boundary(quadtree->internal.se, body))
            child = quadtree->internal.se;
        quadtree_insert(child, body);
        break;
    }
}

void
quadtree_update_mass(struct quadtree *quadtree)
{
    switch (quadtree->type)
    {
    case QUADTREE_EMPTY: break;
    case QUADTREE_EXTERNAL:
        quadtree->total_mass = 0.0;
        quadtree->center_of_mass_x = 0.0;
        quadtree->center_of_mass_y = 0.0;
        for (size_t i = 0; i < quadtree->external.bodies_count; i++)
        {
            quadtree->total_mass += quadtree->external.bodies[i].mass;
            quadtree->center_of_mass_x +=
                quadtree->external.bodies[i].x * quadtree->external.bodies[i].mass;
            quadtree->center_of_mass_y +=
                quadtree->external.bodies[i].y * quadtree->external.bodies[i].mass;
        }
        quadtree->center_of_mass_x /= quadtree->total_mass;
        quadtree->center_of_mass_y /= quadtree->total_mass;
        break;
    case QUADTREE_INTERNAL:
        quadtree_update_mass(quadtree->internal.nw);
        quadtree_update_mass(quadtree->internal.ne);
        quadtree_update_mass(quadtree->internal.sw);
        quadtree_update_mass(quadtree->internal.se);
        quadtree->total_mass =
            quadtree->internal.nw->total_mass + quadtree->internal.ne->total_mass +
            quadtree->internal.sw->total_mass + quadtree->internal.se->total_mass;
        // x center of mass
        quadtree->center_of_mass_x =
            quadtree->internal.nw->center_of_mass_x * quadtree->internal.nw->total_mass +
            quadtree->internal.ne->center_of_mass_x * quadtree->internal.ne->total_mass +
            quadtree->internal.sw->center_of_mass_x * quadtree->internal.sw->total_mass +
            quadtree->internal.se->center_of_mass_x * quadtree->internal.se->total_mass;
        quadtree->center_of_mass_x /= quadtree->total_mass;
        // y center of mass
        quadtree->center_of_mass_y =
            quadtree->internal.nw->center_of_mass_y * quadtree->internal.nw->total_mass +
            quadtree->internal.ne->center_of_mass_y * quadtree->internal.ne->total_mass +
            quadtree->internal.sw->center_of_mass_y * quadtree->internal.sw->total_mass +
            quadtree->internal.se->center_of_mass_y * quadtree->internal.se->total_mass;
        quadtree->center_of_mass_y /= quadtree->total_mass;
        break;
    }
}

static const float approximate_distance_threshold = 0.5;

void
quadtree_force(const struct quadtree *quadtree,
               const struct body     *body,
               const float            gravity,
               float                 *force_x,
               float                 *force_y)
{
    if (quadtree->type == QUADTREE_EMPTY)
        return;
    if (quadtree->type == QUADTREE_EXTERNAL)  // quadtree is a group bodies
    {
#if QUADTREE_MAX_BODIES_COUNT == 8
        body_gravitational_force_avx2(body, quadtree->external.bodies, gravity, force_x, force_y);
#elif QUADTREE_MAX_BODIES_COUNT == 16
        body_gravitational_force_avx2(body, quadtree->external.bodies, gravity, force_x, force_y);
        body_gravitational_force_avx2(body, quadtree->external.bodies + 8, gravity, force_x, force_y);
#elif QUADTREE_MAX_BODIES_COUNT == 32
        body_gravitational_force_avx2(body, quadtree->external.bodies, gravity, force_x, force_y);
        body_gravitational_force_avx2(body, quadtree->external.bodies + 8, gravity, force_x, force_y);
        body_gravitational_force_avx2(body, quadtree->external.bodies + 16, gravity, force_x, force_y);
#endif
        return;
    }
    // Check if we can approximate internal node
    float area_width = fabsf(quadtree->end_x - quadtree->start_x);
    float distance_x = quadtree->center_of_mass_x - body->x;
    float distance_y = quadtree->center_of_mass_y - body->y;
    float inverse_distance = rsqrt(distance_x * distance_x + distance_y * distance_y);
    float ratio = area_width * inverse_distance;
    if (ratio < approximate_distance_threshold)
    {
        body_gravitational_force(body,
                                 &(struct body){.x = quadtree->center_of_mass_x,
                                                .y = quadtree->center_of_mass_y,
                                                .mass = quadtree->total_mass},
                                 gravity,
                                 force_x,
                                 force_y);
        return;
    }
    // Compute force for all region
    float nw_force_x = 0.0, nw_force_y = 0.0, ne_force_x = 0.0, ne_force_y = 0.0, sw_force_x = 0.0,
          sw_force_y = 0.0, se_force_x = 0.0, se_force_y = 0.0;
    quadtree_force(quadtree->internal.nw, body, gravity, &nw_force_x, &nw_force_y);
    quadtree_force(quadtree->internal.ne, body, gravity, &ne_force_x, &ne_force_y);
    quadtree_force(quadtree->internal.sw, body, gravity, &sw_force_x, &sw_force_y);
    quadtree_force(quadtree->internal.se, body, gravity, &se_force_x, &se_force_y);
    *force_x = nw_force_x + ne_force_x + sw_force_x + se_force_x;
    *force_y = nw_force_y + ne_force_y + sw_force_y + se_force_y;
}

void
quadtree_stats(const struct quadtree *quadtree, struct quadtree_stats *stats)
{
    stats->node_count++;
    switch (quadtree->type)
    {
    case QUADTREE_EMPTY: stats->empty_count++; break;
    case QUADTREE_EXTERNAL: stats->external_count++; break;
    case QUADTREE_INTERNAL:
        stats->internal_count++;
        quadtree_stats(quadtree->internal.nw, stats);
        quadtree_stats(quadtree->internal.ne, stats);
        quadtree_stats(quadtree->internal.sw, stats);
        quadtree_stats(quadtree->internal.se, stats);
        break;
    }
}
