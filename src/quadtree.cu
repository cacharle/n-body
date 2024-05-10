#include <cmath>
#include "body.h"

float quadtree_max_x = -INFINITY;
float quadtree_min_x = +INFINITY;
float quadtree_max_y = -INFINITY;
float quadtree_min_y = +INFINITY;

__global__ void cu_quadtree_bounding_box(const struct body *bodies)
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (bodies[i].x > max_x)
        atomicCAS((int*)&max_x, *(int*)&bodies[i].x, *(int*)&bodies[i].x);
    if (bodies[i].x < min_x)
        atomicCAS((int*)&min_x, *(int*)&bodies[i].x, *(int*)&bodies[i].x);
    if (bodies[i].y > max_y)
        atomicCAS((int*)&max_y, *(int*)&bodies[i].y, *(int*)&bodies[i].y);
    if (bodies[i].y < min_y)
        atomicCAS((int*)&min_y, *(int*)&bodies[i].y, *(int*)&bodies[i].y);
}

// void init_quadtree_root(struct quadtree *root, const struct body *bodies)
// {
//     cu_quadtree_bounding_box<<<1, 1024>>>(bodies);
//     cudaDeviceSynchronize();
//     root->start_x = quadtree_min_x;
//     root->end_x = quadtree_max_x;
//     root->start_y = quadtree_min_y;
//     root->end_y = quadtree_max_y;
// }
