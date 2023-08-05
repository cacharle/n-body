#define _XOPEN_SOURCE
#include "body.h"
#include "draw.h"
#include "quadtree.h"
#include "utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// #define BODIES_COUNT 50000
static size_t             bodies_count = 1000;
static struct body       *bodies = NULL;
static const float        time_step = 0.001f;
static size_t             threads_count = 1;
static pthread_t         *threads = NULL;
static struct worker_arg *threads_args = NULL;
static float              gravity = 0.0005f;
static bool               flag_mass = false;
static bool               flag_debug = false;
static bool               flag_black_hole = false;
static void (*flag_bodies_initialization_function)(struct body *) = body_init_random_circle;

struct worker_arg
{
    size_t           start_index;
    size_t           stop_index;
    struct quadtree *quadtree;
};

static void *
worker_func(struct worker_arg *arg)
{
    for (size_t i = arg->start_index; i < arg->stop_index; i++)
    {
        // body_acceleration(&bodies[i]);
        float acceleration_x = 0.0, acceleration_y = 0.0;
        quadtree_force(arg->quadtree, &bodies[i], gravity, &acceleration_x, &acceleration_y);
        acceleration_x /= bodies[i].mass;
        acceleration_y /= bodies[i].mass;
        bodies[i].velocity_y -= acceleration_y * time_step;
        bodies[i].velocity_x -= acceleration_x * time_step;
        bodies[i].x += bodies[i].velocity_x * time_step;
        bodies[i].y += bodies[i].velocity_y * time_step;
    }
    return NULL;
}

extern void
update_bodies_naive(struct body *bodies_cpu, size_t bodies_count, float gravity);

int
main(int argc, char **argv)
{
    threads_count = sysconf(_SC_NPROCESSORS_ONLN);
    int option;
    while ((option = getopt(argc, argv, "hb:ow:mi:g:d")) != -1)
    {
        switch (option)
        {
        case 'h':
            printf("Usage: n-body\n"
                   "\t-h Print this message\n"
                   "\t-b Number of body (default: %zu)\n"
                   "\t-o Add a \"black hole\" in the center\n"
                   "\t-w Number of workers (default: number of cpu cores)\n"
                   "\t-m Assign a random mass to bodies and draw that mass\n"
                   "\t\t(all bodies have the same mass by default)\n"
                   "\t-i Body initialization method (default: circle)\n"
                   "\t\tAvailable: uniform, circle\n"
                   "\t-g Gravity (default: %f)\n"
                   "\t-d Enable debug mode\n"
                   "UI Controls:\n"
                   "\tEscape/Q: Quit\n"
                   "\tSpace:    Pause\n",
                   bodies_count,
                   gravity);
            exit(EXIT_SUCCESS);
            break;
        case 'b':
            errno = 0;
            bodies_count = strtoul(optarg, NULL, 10);
            if (errno != 0)
                die("Invalid argument to -b: %s", optarg);
            break;
        case 'o':
            flag_black_hole = true;
            break;
        case 'w':
            errno = 0;
            threads_count = strtoul(optarg, NULL, 10);
            if (errno != 0)
                die("Invalid argument to -w: %s", optarg);
            break;
        case 'm': flag_mass = true; break;
        case 'i':
            if (strcmp(optarg, "uniform") == 0)
                flag_bodies_initialization_function = body_init_random_uniform;
            else if (strcmp(optarg, "circle") == 0)
                flag_bodies_initialization_function = body_init_random_circle;
            else if (strcmp(optarg, "thorus") == 0)
                flag_bodies_initialization_function = body_init_random_thorus;
            else
                die("'%s' is not a valid body initialization", optarg);
            break;
        case 'g':
            errno = 0;
            gravity = strtod(optarg, NULL);
            if (errno != 0)
                die("Invalid argument to -w: %s", optarg);
            break;
        case 'd': flag_debug = true; break;
        }
    }
    if (flag_black_hole)
        bodies_count++;

    // Get a random seed from the system
    FILE *random_file = fopen("/dev/random", "r");
    if (random_file == NULL)
        die("Cannot open /dev/random");
    unsigned int seed;
    fread(&seed, sizeof seed, 1, random_file);
    if (ferror(random_file))
        die("Cannot read /dev/random");
    fclose(random_file);
    srand(seed);
    // Initialize the bodies
    bodies = xmalloc(sizeof(struct body) * bodies_count);
    for (size_t i = 0; i < bodies_count; i++)
    {
        flag_bodies_initialization_function(&bodies[i]);
        if (flag_mass)
            bodies[i].mass = frand() + 0.3;
    }
    if (flag_black_hole)
    {
        bodies[0].x = 0.5;
        bodies[0].y = 0.5;
        bodies[0].mass = 100.0f;
    }

    // Initialize threads
    threads = xmalloc(sizeof(pthread_t *) * threads_count);
    threads_args = xmalloc(sizeof(struct worker_arg) * threads_count);

    draw_init();
    bool running = true;
    bool paused = false;
    while (running)
    {
        draw_handle_events(&running, &paused);
        if (paused)
        {
            SDL_Delay(10);
            continue;
        }
        // update_bodies_naive(bodies, bodies_count, gravity);
        // Create a quadtree
        struct quadtree *bodies_quadtree = quadtree_new();
        bodies_quadtree->start_x = -1.0;
        bodies_quadtree->start_y = -1.0;
        bodies_quadtree->end_x = 2.0;
        bodies_quadtree->end_y = 2.0;
        for (size_t i = 0; i < bodies_count; i++)
            quadtree_insert(bodies_quadtree, bodies[i]);
        quadtree_update_mass(bodies_quadtree);
        if (flag_debug)
        {
            struct quadtree_stats stats = {0};
            quadtree_stats(bodies_quadtree, &stats);
            printf(
                "stats:\n"
                "\tnode count:     %5zu\n"
                "\tempty count:    %5zu\n"
                "\texternal count: %5zu, average bodies in external %5.1f\n"
                "\tInternal count: %5zu\n",
                stats.node_count,
                stats.empty_count,
                stats.external_count,
                (double)bodies_count / (double)stats.external_count,
                stats.internal_count
            );
        }
        // Create threads to compute the gravitational forces
        size_t stride = bodies_count / threads_count;
        for (size_t i = 0, start_index = 0; i < threads_count; i++, start_index += stride)
        {
            threads_args[i] = (struct worker_arg){
                .start_index = start_index,
                .stop_index = start_index + stride,
                .quadtree = bodies_quadtree,
            };
            pthread_create(&threads[i], NULL, (void *(*)(void *))worker_func, &threads_args[i]);
        }
        for (size_t i = 0; i < threads_count; i++)
            pthread_join(threads[i], NULL);
        draw_update(bodies, bodies_count, flag_mass, flag_debug ? bodies_quadtree : NULL);
        // // draw_update(bodies, bodies_count, flag_mass, NULL);
        quadtree_destroy(bodies_quadtree);
        // SDL_Delay(100);
    }
    free(threads);
    free(threads_args);
    free(bodies);
    draw_quit();
    return EXIT_SUCCESS;
}
