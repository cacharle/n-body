#include "body.h"
#include <string.h>
#include <stdio.h>

#define CUDA_CHECK(x) do {                                                      \
        cudaError_t cuda_check_result;                                          \
        if ((cuda_check_result = (x)) != cudaSuccess)                           \
        {                                                                       \
            fprintf(stderr, "Error CUDA: %s:%d: %s\n\t%s",                      \
                __FILE__, __LINE__, cudaGetErrorString(cuda_check_result), #x); \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while(0);

const float time_step = 0.001f;

// __global__ void bodies_gravitational_force_update(
//     struct body *bodies,
//     struct body *bodies_origin,
//     size_t bodies_count,
//     float gravity
// ) {
//     size_t b1_id = blockIdx.x * blockDim.x + threadIdx.x;
//     size_t b2_id = blockIdx.y * blockDim.y + threadIdx.y;
//     if (b1_id >= bodies_count || b2_id >= bodies_count)
//         return;
//     struct body *b1 = &bodies_origin[b1_id];
//     struct body *b2 = &bodies_origin[b2_id];
//     if (fabsf(b1->x - b2->x) < 0.01f || fabsf(b1->y - b2->y) < 0.01f)
//         return;
//     float distance_x = b1->x - b2->x;
//     float distance_y = b1->y - b2->y;
//     float distance_square = distance_x * distance_x + distance_y * distance_y;
//     float force = (b1->mass * b2->mass * gravity) / distance_square;
//     float dx = b1->x - b2->x;
//     float dy = b1->y - b2->y;
//     float magnitude_inverse = 1.0 / sqrt(dx * dx + dy * dy);
//     dx *= magnitude_inverse;
//     dy *= magnitude_inverse;
//     dx *= force;
//     dy *= force;
//
//     if (!isnan(dx) && !isnan(dy))
//     {
//         bodies[b1_id].acceleration_x += dx;
//         bodies[b1_id].acceleration_y += dy;
//     }
// }
//
// __global__ void update_bodies_position(struct body *bodies)
// {
//     size_t i = blockIdx.x * blockDim.x + threadIdx.x;
//     bodies[i].acceleration_x /= bodies[i].mass;
//     bodies[i].acceleration_y /= bodies[i].mass;
//     bodies[i].velocity_y -= bodies[i].acceleration_y * time_step;
//     bodies[i].velocity_x -= bodies[i].acceleration_x * time_step;
//     bodies[i].x += bodies[i].velocity_x * time_step;
//     bodies[i].y += bodies[i].velocity_y * time_step;
// }

// extern "C" void update_bodies_naive(struct body *bodies_host, size_t bodies_count, float gravity) {
//     for (size_t i = 0; i < bodies_count; i++)
//     {
//         bodies_host[i].acceleration_x = 0.0;
//         bodies_host[i].acceleration_y = 0.0;
//     }
//
//     size_t bodies_bytes = sizeof(struct body) * bodies_count;
//     if (bodies == NULL || bodies_origin == NULL)
//     {
//         CUDA_CHECK(cudaMalloc(&bodies, bodies_bytes));
//         CUDA_CHECK(cudaMalloc(&bodies_origin, bodies_bytes));
//     }
//     CUDA_CHECK(cudaMemcpy(bodies, bodies_host, bodies_bytes, cudaMemcpyHostToDevice));
//     CUDA_CHECK(cudaMemcpy(bodies_origin, bodies, bodies_bytes, cudaMemcpyDeviceToDevice));
//
//     size_t threads_count = 32;
//     size_t blocks_count = (bodies_count + threads_count - 1) / threads_count;
//     dim3 threads_dim(threads_count, threads_count);
//     dim3 blocks_dim(blocks_count, blocks_count);
//     bodies_gravitational_force_update<<<blocks_dim, threads_dim>>>(
//         bodies,
//         bodies_origin,
//         bodies_count,
//         gravity
//     );
//     cudaDeviceSynchronize();
//     threads_count = 256;
//     blocks_count = (bodies_count + threads_count - 1) / threads_count;
//     threads_dim = dim3(threads_count);
//     blocks_dim = dim3(blocks_count);
//     update_bodies_position<<<blocks_dim, threads_dim>>>(bodies);
//
//     CUDA_CHECK(cudaMemcpy(bodies_host, bodies, bodies_bytes, cudaMemcpyDeviceToHost));
// }

// #include <thrust/extrema.h>

__global__ void bodies_struct_to_arrays(
    struct body *bodies,
    float *bodies_x,
    float *bodies_y,
    float *bodies_velocity_x,
    float *bodies_velocity_y)
{
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    bodies_x[idx] = bodies[idx].x;
    bodies_y[idx] = bodies[idx].y;
    bodies_velocity_x[idx] = bodies[idx].velocity_x;
    bodies_velocity_y[idx] = bodies[idx].velocity_y;
}

#define THREADS_COUNT 256

__global__ void min_max_element_kernel(float *xs, size_t count, float *mins, float *maxs)
{
    __shared__ float partial_min[THREADS_COUNT];
    __shared__ float partial_max[THREADS_COUNT];
    size_t idx = threadIdx.x + blockIdx.x * blockDim.x;
    // Current minimum of the binary tree level (start of the level range)
    partial_min[threadIdx.x] = (idx < count) ? xs[idx] : INFINITY;
    partial_max[threadIdx.x] = (idx < count) ? xs[idx] : -INFINITY;
    __syncthreads();
    // Compare the current element with the powers of 2 (min/max solved by another thread)
    // for (int stride = 1; stride < blockDim.x; stride <<= 1)
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) // Better for data locality than the above?
    {
        // if (threadIdx.x % stride == 0) {
        if (threadIdx.x < stride) {
            partial_min[threadIdx.x] = min(partial_min[threadIdx.x], partial_min[threadIdx.x + stride]);
            partial_max[threadIdx.x] = max(partial_max[threadIdx.x], partial_max[threadIdx.x + stride]);
        }
        __syncthreads();
    }
    // Set the min/max element in the *global* memory, the first thread of the block ends up with
    // the min/max element
    if (threadIdx.x == 0)
    {
        mins[blockIdx.x] = partial_min[0];
        maxs[blockIdx.x] = partial_max[0];
    }
    __syncthreads();
    // Lastly the first thread of the first block computes the min/max of all the other blocks
    if (blockIdx.x == 0 && threadIdx.x == 0)
    {
        for (size_t i = 1; i < gridDim.x; i++)
        {
            mins[0] = min(mins[0], mins[i]);
            maxs[0] = max(maxs[0], maxs[i]);
        }
    }
}

void min_max_element(float *xs, size_t count, float *pmin, float *pmax)
{
    size_t blocks_count = (count + THREADS_COUNT - 1) / THREADS_COUNT;
    dim3 threads_dim = dim3(THREADS_COUNT);
    dim3 blocks_dim = dim3(blocks_count);
    float *mins, *maxs;
    cudaMalloc(&mins, sizeof(float) * blocks_count);
    cudaMalloc(&maxs, sizeof(float) * blocks_count);
    min_max_element_kernel<<<blocks_dim, threads_dim>>>(xs, count, mins, maxs);
    cudaMemcpy(pmin, mins, sizeof(float), cudaMemcpyDeviceToHost);
    cudaMemcpy(pmax, maxs, sizeof(float), cudaMemcpyDeviceToHost);
    cudaFree(mins);
    cudaFree(maxs);
}

extern "C" void update_bodies_barnes_hut(struct body *bodies_host, size_t bodies_count, float gravity)
{

    static struct body *bodies = NULL, *bodies_origin = NULL;
    static float *bodies_x = NULL, *bodies_y = NULL, *bodies_velocity_x = NULL, *bodies_velocity_y = NULL;

    size_t bodies_bytes = sizeof(struct body) * bodies_count;
    if (bodies == NULL || bodies_origin == NULL)
    {
        CUDA_CHECK(cudaMalloc(&bodies, bodies_bytes));
        CUDA_CHECK(cudaMalloc(&bodies_origin, bodies_bytes));

        CUDA_CHECK(cudaMalloc(&bodies_x, bodies_count * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&bodies_y, bodies_count * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&bodies_velocity_x, bodies_count * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&bodies_velocity_y, bodies_count * sizeof(float)));
    }

    CUDA_CHECK(cudaMemcpy(bodies, bodies_host, bodies_bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(bodies_origin, bodies, bodies_bytes, cudaMemcpyDeviceToDevice));

    cudaDeviceSynchronize();

    size_t blocks_count = (bodies_count + THREADS_COUNT - 1) / THREADS_COUNT;
    // printf("%zu*%zu = %zu\n", threads_count, blocks_count, threads_count * blocks_count);
    dim3 threads_dim = dim3(THREADS_COUNT);
    dim3 blocks_dim = dim3(blocks_count);
    bodies_struct_to_arrays<<<blocks_dim, threads_dim>>>(
        bodies,
        bodies_x,
        bodies_y,
        bodies_velocity_x,
        bodies_velocity_y
    );

    float x_min, x_max, y_min, y_max;
    min_max_element(bodies_x, bodies_count, &x_min, &x_max);
    min_max_element(bodies_y, bodies_count, &y_min, &y_max);
    printf("%.2f, %.2f -> %.2f, %.2f\n", x_min, y_min, x_max, y_max);


}


