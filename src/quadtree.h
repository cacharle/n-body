#ifndef QUADTREE_H
#define QUADTREE_H

#include "body.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum quadtree_type
{
    QUADTREE_EMPTY = 0,
    QUADTREE_EXTERNAL = 1,
    QUADTREE_INTERNAL = 2,
};

#define QUADTREE_MAX_BODIES_COUNT 8

struct quadtree
{
    enum quadtree_type type;
    float              total_mass;
    float              center_of_mass_x;
    float              center_of_mass_y;
    float              start_x;
    float              start_y;
    float              end_x;
    float              end_y;
    union
    {
        struct
        {
            struct body bodies[QUADTREE_MAX_BODIES_COUNT];
            size_t      bodies_count;
        } external;
        struct
        {
            struct quadtree *nw;
            struct quadtree *ne;
            struct quadtree *sw;
            struct quadtree *se;
        } internal;
    };
};

struct quadtree_stats
{
    size_t node_count;
    size_t empty_count;
    size_t external_count;
    size_t internal_count;
};

struct quadtree *
quadtree_new(void);
void
quadtree_destroy(struct quadtree *quadtree);
void
quadtree_insert(struct quadtree *quadtree, struct body body);
void
quadtree_update_mass(struct quadtree *quadtree);
void
quadtree_force(const struct quadtree *quadtree,
               const struct body     *body,
               const float            gravity,
               float                 *force_x,
               float                 *force_y);
void
quadtree_stats(const struct quadtree *quadtree, struct quadtree_stats *stats);

#endif
