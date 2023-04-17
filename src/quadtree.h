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

struct quadtree
{
    enum quadtree_type type;
    double             total_mass;
    double             center_of_mass_x;
    double             center_of_mass_y;
    double             start_x;
    double             start_y;
    double             end_x;
    double             end_y;
    union
    {
        struct body body;
        struct
        {
            struct quadtree *nw;
            struct quadtree *ne;
            struct quadtree *sw;
            struct quadtree *se;
        } children;
    };
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
               double                *force_x,
               double                *force_y);

#endif
