#include "quadtree.h"
#include "body.h"

static bool
in_boundary(struct quadtree *quadtree, struct body body)
{
    return body.x <= quadtree->end_x && body.x >= quadtree->start_x && body.y <= quadtree->end_y &&
           body.y >= quadtree->start_y;
}

struct quadtree *
quadtree_new(void)
{
    struct quadtree *quadtree = malloc(sizeof(struct quadtree));
    assert(quadtree != NULL);
    memset(quadtree, 0, sizeof *quadtree);
    quadtree->type = QUADTREE_EMPTY;
    return quadtree;
}

void
quadtree_destroy(struct quadtree *quadtree)
{
    if (quadtree->type == QUADTREE_INTERNAL)
    {
        quadtree_destroy(quadtree->children.nw);
        quadtree_destroy(quadtree->children.ne);
        quadtree_destroy(quadtree->children.sw);
        quadtree_destroy(quadtree->children.se);
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
        quadtree->body = body;
        break;
    case QUADTREE_EXTERNAL:
        quadtree->type = QUADTREE_INTERNAL;
        struct body original_body = quadtree->body;
        quadtree->children.nw = quadtree_new();
        quadtree->children.ne = quadtree_new();
        quadtree->children.sw = quadtree_new();
        quadtree->children.se = quadtree_new();
        double mid_x = quadtree->start_x + (quadtree->end_x - quadtree->start_x) / 2.0;
        double mid_y = quadtree->start_y + (quadtree->end_y - quadtree->start_y) / 2.0;
        // nw
        quadtree->children.nw->start_x = quadtree->start_x;
        quadtree->children.nw->end_x = mid_x;
        quadtree->children.nw->start_y = quadtree->start_y;
        quadtree->children.nw->end_y = mid_y;
        // ne
        quadtree->children.ne->start_x = mid_x;
        quadtree->children.ne->end_x = quadtree->end_x;
        quadtree->children.ne->start_y = quadtree->start_y;
        quadtree->children.ne->end_y = mid_y;
        // sw
        quadtree->children.sw->start_x = quadtree->start_x;
        quadtree->children.sw->end_x = mid_x;
        quadtree->children.sw->start_y = mid_y;
        quadtree->children.sw->end_y = quadtree->end_y;
        // se
        quadtree->children.se->start_x = mid_x;
        quadtree->children.se->end_x = quadtree->end_x;
        quadtree->children.se->start_y = mid_y;
        quadtree->children.se->end_y = quadtree->end_y;
        quadtree_insert(quadtree, original_body);  // reinsert the original body
        quadtree_insert(quadtree, body);           // treated as an internal node now
        break;
    case QUADTREE_INTERNAL:
        if (in_boundary(quadtree->children.nw, body))
            child = quadtree->children.nw;
        else if (in_boundary(quadtree->children.ne, body))
            child = quadtree->children.ne;
        else if (in_boundary(quadtree->children.sw, body))
            child = quadtree->children.sw;
        else if (in_boundary(quadtree->children.se, body))
            child = quadtree->children.se;
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
        quadtree->total_mass = quadtree->body.mass;
        quadtree->center_of_mass_x = quadtree->body.x;
        quadtree->center_of_mass_y = quadtree->body.y;
        break;
    case QUADTREE_INTERNAL:
        quadtree_update_mass(quadtree->children.nw);
        quadtree_update_mass(quadtree->children.ne);
        quadtree_update_mass(quadtree->children.sw);
        quadtree_update_mass(quadtree->children.se);
        quadtree->total_mass =
            quadtree->children.nw->total_mass + quadtree->children.ne->total_mass +
            quadtree->children.sw->total_mass + quadtree->children.se->total_mass;
        // x center of mass
        quadtree->center_of_mass_x =
            quadtree->children.nw->center_of_mass_x * quadtree->children.nw->total_mass +
            quadtree->children.ne->center_of_mass_x * quadtree->children.ne->total_mass +
            quadtree->children.sw->center_of_mass_x * quadtree->children.sw->total_mass +
            quadtree->children.se->center_of_mass_x * quadtree->children.se->total_mass;
        quadtree->center_of_mass_x /= quadtree->total_mass;
        // y center of mass
        quadtree->center_of_mass_y =
            quadtree->children.nw->center_of_mass_y * quadtree->children.nw->total_mass +
            quadtree->children.ne->center_of_mass_y * quadtree->children.ne->total_mass +
            quadtree->children.sw->center_of_mass_y * quadtree->children.sw->total_mass +
            quadtree->children.se->center_of_mass_y * quadtree->children.se->total_mass;
        quadtree->center_of_mass_y /= quadtree->total_mass;
        break;
    }
}

static const double approximate_distance_threshold = 0.0;

void
quadtree_force(struct quadtree *quadtree, struct body body, double *force_x, double *force_y)
{
    double area_width = fabs(quadtree->end_x - quadtree->start_x);
    double distance_x = quadtree->center_of_mass_x - body.x;
    double distance_y = quadtree->center_of_mass_y - body.y;
    double distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    double ratio = area_width / distance;
    printf("here %f\n", ratio);
    if (ratio > approximate_distance_threshold)
    {
        body_gravitational_force(body,
                                 (struct body){.x = quadtree->center_of_mass_x,
                                               .y = quadtree->center_of_mass_y,
                                               .mass = quadtree->total_mass},
                                 force_x,
                                 force_y);
        return;
    }
    double nw_force_x, nw_force_y, ne_force_x, ne_force_y, sw_force_x, sw_force_y, se_force_x,
        se_force_y;
    switch (quadtree->type)
    {
    case QUADTREE_EMPTY: break;
    case QUADTREE_EXTERNAL: body_gravitational_force(body, quadtree->body, force_x, force_y); break;
    case QUADTREE_INTERNAL:
        quadtree_force(quadtree->children.nw, body, &nw_force_x, &nw_force_y);
        quadtree_force(quadtree->children.ne, body, &ne_force_x, &ne_force_y);
        quadtree_force(quadtree->children.sw, body, &sw_force_x, &sw_force_y);
        quadtree_force(quadtree->children.se, body, &se_force_x, &se_force_y);
        *force_x = nw_force_x + ne_force_x + sw_force_x + se_force_x;
        *force_y = nw_force_y + ne_force_y + sw_force_y + se_force_y;
        break;
    }
}
