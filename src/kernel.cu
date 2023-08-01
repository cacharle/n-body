#include "body.h"

const float time_step = 0.001f;

__global__ void bodies_gravitational_force_update(struct body *bodies, struct body *bodies_origin, size_t bodies_count, float gravity) {
    size_t b1_id = blockIdx.x * blockDim.x + threadIdx.x;
    size_t b2_id = blockIdx.y * blockDim.y + threadIdx.y;
    if (b1_id >= bodies_count || b2_id >= bodies_count)
        return;
    struct body *b1 = &bodies_origin[b1_id];
    struct body *b2 = &bodies_origin[b2_id];
    if (fabsf(b1->x - b2->x) < 0.01f || fabsf(b1->y - b2->y) < 0.01f)
        return;
    float distance_x = b1->x - b2->x;
    float distance_y = b1->y - b2->y;
    float distance_square = distance_x * distance_x + distance_y * distance_y;
    float force = (b1->mass * b2->mass * gravity) / distance_square;
    float dx = b1->x - b2->x;
    float dy = b1->y - b2->y;
    float magnitude_inverse = 1.0 / sqrt(dx * dx + dy * dy);
    dx *= magnitude_inverse;
    dy *= magnitude_inverse;
    dx *= force;
    dy *= force;

    if (!isnan(dx) && !isnan(dy))
    {
        bodies[b1_id].acceleration_x += dx;
        bodies[b1_id].acceleration_y += dy;
    }
}

__global__ void update_bodies_position(struct body *bodies)
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    bodies[i].acceleration_x /= bodies[i].mass;
    bodies[i].acceleration_y /= bodies[i].mass;
    bodies[i].velocity_y -= bodies[i].acceleration_y * time_step;
    bodies[i].velocity_x -= bodies[i].acceleration_x * time_step;
    bodies[i].x += bodies[i].velocity_x * time_step;
    bodies[i].y += bodies[i].velocity_y * time_step;
}

#define CUDA_CHECK(x) do {                                                      \
        cudaError_t cuda_check_result;                                          \
        if ((cuda_check_result = (x)) != cudaSuccess)                           \
        {                                                                       \
            fprintf(stderr, "Error CUDA: %s:%d: %s\n\t%s",                      \
                __FILE__, __LINE__, cudaGetErrorString(cuda_check_result), #x); \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while(0);

struct body *bodies = NULL, *bodies_origin = NULL;

extern "C" void update_bodies_naive(struct body *bodies_host, size_t bodies_count, float gravity) {
    for (size_t i = 0; i < bodies_count; i++)
    {
        bodies_host[i].acceleration_x = 0.0;
        bodies_host[i].acceleration_y = 0.0;
    }

    size_t bodies_bytes = sizeof(struct body) * bodies_count;
    if (bodies == NULL || bodies_origin == NULL)
    {
        CUDA_CHECK(cudaMalloc(&bodies, bodies_bytes));
        CUDA_CHECK(cudaMalloc(&bodies_origin, bodies_bytes));
    }
    CUDA_CHECK(cudaMemcpy(bodies, bodies_host, bodies_bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(bodies_origin, bodies, bodies_bytes, cudaMemcpyDeviceToDevice));

    size_t threads_count = 32;
    size_t blocks_count = (bodies_count + threads_count - 1) / threads_count;
    dim3 threads_dim(threads_count, threads_count);
    dim3 blocks_dim(blocks_count, blocks_count);
    bodies_gravitational_force_update<<<blocks_dim, threads_dim>>>(bodies, bodies_origin, bodies_count, gravity);

    threads_count = 256;
    blocks_count = (bodies_count + threads_count - 1) / threads_count;
    threads_dim = dim3(threads_count);
    blocks_dim = dim3(blocks_count);
    update_bodies_position<<<blocks_dim, threads_dim>>>(bodies);

    CUDA_CHECK(cudaMemcpy(bodies_host, bodies, bodies_bytes, cudaMemcpyDeviceToHost));
}
