#include "body.h"

__global__ void bodies_gravitational_force_update(struct body *bodies, struct body *bodies_origin, float gravity) {
    struct body *b1 = &bodies_origin[blockIdx.x];
    struct body *b2 = &bodies_origin[threadIdx.x];
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
    float magnitude_inverse = 1.0 / sqrt(dx * dx + dy * dy);
    dx *= magnitude_inverse;
    dy *= magnitude_inverse;
    dx *= force;
    dy *= force;
}

inline
cudaError_t checkCuda(cudaError_t result)
{
    if (result != cudaSuccess)
    {
        fprintf(stderr, "Error CUDA: %s\n", cudaGetErrorString(result));
        exit(EXIT_FAILURE);
    }
    return result;
}

extern "C" void update_bodies_naive(struct body *bodies_cpu, size_t bodies_count, float gravity) {
    struct body *bodies, *bodies_origin;
    size_t bodies_bytes = sizeof(struct body) * bodies_count;
    checkCuda(cudaMalloc(&bodies, bodies_bytes));
    checkCuda(cudaMalloc(&bodies_origin, bodies_bytes));
    checkCuda(cudaMemcpy(bodies, bodies_cpu, bodies_bytes, cudaMemcpyHostToDevice));
    checkCuda(cudaMemcpy(bodies_origin, bodies, bodies_bytes, cudaMemcpyDeviceToDevice));

    bodies_gravitational_force_update<<<1, 1>>>(bodies, bodies_origin, gravity);

    cudaFree(bodies);
    cudaFree(bodies_origin);
}
