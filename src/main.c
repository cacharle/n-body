#include "body.h"
#include "quadtree.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <bits/stdint-uintn.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define SDL_ASSERT_NO_ERROR                                                           \
    do                                                                                \
    {                                                                                 \
        if (*SDL_GetError() != '\0')                                                  \
        {                                                                             \
            SDL_Log("[ERROR SDL] %s at %s:%d\n", SDL_GetError(), __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                       \
        }                                                                             \
    } while (false);

// SDL2
static const uint32_t window_width = 1000;
static const uint32_t window_height = 1000;
static SDL_Renderer  *renderer = NULL;
static const char    *font_path = "/usr/share/fonts/noto/NotoSansMono-SemiBold.ttf";
// bodies
#define BODIES_COUNT 100000
static struct body bodies_previous[BODIES_COUNT] = {0};
static struct body bodies[BODIES_COUNT] = {0};

static const double time_step = 0.001;

void
die(void)
{
    perror("n-body");
    exit(EXIT_FAILURE);
}

void
body_acceleration(struct body *body)
{
    // Compute the attraction force of every other bodies to this body (and
    // accumulate it in acceleration)
    double acceleration_x_ = 0.0;
    double acceleration_y_ = 0.0;
    for (size_t j = 0; j < BODIES_COUNT; j++)
    {
        // Cheating a bit with physics by ignoring when bodies collide (they go to infinity and
        // beyond otherwise)
        // TODO: implement collision detection: https://www.youtube.com/watch?v=5cmNxz9gTFA
        if (body == &bodies_previous[j] || fabs(body->x - bodies_previous[j].x) < 0.03 ||
            fabs(body->y - bodies_previous[j].y) < 0.03)
            continue;
        double force_x, force_y;
        body_gravitational_force(body, &bodies_previous[j], &force_x, &force_y);
        acceleration_x_ -= force_x;
        acceleration_y_ -= force_y;
    }
    body->velocity_y += acceleration_y_;
    body->velocity_x += acceleration_x_;
}

#define THREAD_POOL_COUNT 4
pthread_t thread_pool[THREAD_POOL_COUNT];
#define BODIES_COUNT_PER_THREAD (BODIES_COUNT / THREAD_POOL_COUNT)

struct worker_arg
{
    size_t           start_index;
    size_t           stop_index;
    struct quadtree *quadtree;
};

void *
worker_func(struct worker_arg *arg)
{
    for (size_t i = arg->start_index; i < arg->stop_index; i++)
    {
        // body_acceleration(&bodies[i]);
        double acceleration_x = 0.0, acceleration_y = 0.0;
        quadtree_force(arg->quadtree, &bodies[i], &acceleration_x, &acceleration_y);
        acceleration_x /= bodies[i].mass;
        acceleration_y /= bodies[i].mass;
        bodies[i].velocity_y -= acceleration_y * time_step;
        bodies[i].velocity_x -= acceleration_x * time_step;
        bodies[i].x += bodies[i].velocity_x * time_step;
        bodies[i].y += bodies[i].velocity_y * time_step;
    }
    return NULL;
}

// void
// update_bodies(struct quadtree *quadtree)
// {
//     memcpy(bodies_previous, bodies, sizeof bodies);
//     for (size_t i = 0; i < BODIES_COUNT; i++)
//     {
//         // body_acceleration(&bodies[i]);
//         double acceleration_x = 0.0, acceleration_y = 0.0;
//         quadtree_force(quadtree, &bodies[i], &acceleration_x, &acceleration_y);
//         acceleration_x /= bodies[i].mass;
//         acceleration_y /= bodies[i].mass;
//         bodies[i].velocity_y -= acceleration_y * time_step;
//         bodies[i].velocity_x -= acceleration_x * time_step;
//         bodies[i].x += bodies[i].velocity_x * time_step;
//         bodies[i].y += bodies[i].velocity_y * time_step;
//     }
// }

static SDL_Point bodies_points[BODIES_COUNT] = {0};

void
draw_bodies()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    size_t points_i = 0;
    for (size_t i = 0; i < BODIES_COUNT; i++)
    {
        uint32_t canvas_x = bodies[i].x * (double)window_width;
        uint32_t canvas_y = bodies[i].y * (double)window_height;
        if (canvas_x < 0 || canvas_x > window_width || canvas_y < 0 || canvas_y > window_height)
            continue;
        bodies_points[points_i].x = canvas_x;
        bodies_points[points_i].y = canvas_y;
        points_i++;
        // uint32_t radius = 30 * bodies[i].mass;
        // aacircleRGBA(renderer, canvas_x, canvas_y, radius, 255, 255, 255, 255);
    }
    SDL_RenderDrawPoints(renderer, bodies_points, points_i);
}

void
draw_quadtree(struct quadtree *quadtree, unsigned int depth)
{
    // if (depth > 8)
    //     return;
    uint32_t canvas_start_x = quadtree->start_x * (double)window_width;
    uint32_t canvas_start_y = quadtree->start_y * (double)window_height;
    uint32_t canvas_end_x = quadtree->end_x * (double)window_width;
    uint32_t canvas_end_y = quadtree->end_y * (double)window_height;
    uint32_t canvas_center_of_mass_x = quadtree->center_of_mass_x * (double)window_width;
    uint32_t canvas_center_of_mass_y = quadtree->center_of_mass_y * (double)window_height;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
    SDL_RenderDrawLine(renderer, canvas_start_x, canvas_start_y, canvas_end_x, canvas_start_y);
    SDL_RenderDrawLine(renderer, canvas_start_x, canvas_start_y, canvas_start_x, canvas_end_y);
    // if (quadtree->type == QUADTREE_EMPTY)
    // {
    //     SDL_SetRenderDrawColor(renderer, 20, 20, 20, 0);
    //     SDL_RenderFillRect(
    //         renderer,
    //         &(SDL_Rect){
    //             .x = canvas_start_x + 1,
    //             .y = canvas_start_y + 1,
    //             .w = canvas_end_x - canvas_start_x - 1,
    //             .h = canvas_end_y - canvas_start_y - 1
    //         }
    //     );
    // }
    if (quadtree->type == QUADTREE_INTERNAL)
    {
        // filledCircleColor(
        //     renderer, canvas_center_of_mass_x, canvas_center_of_mass_y, 3, 0xff0000ff);
        draw_quadtree(quadtree->children.nw, depth + 1);
        draw_quadtree(quadtree->children.ne, depth + 1);
        draw_quadtree(quadtree->children.sw, depth + 1);
        draw_quadtree(quadtree->children.se, depth + 1);
    }
}

int
main()
{
    FILE *random_file = fopen("/dev/random", "r");
    if (random_file == NULL)
        die();
    unsigned int seed;
    fread(&seed, sizeof seed, 1, random_file);
    if (ferror(random_file))
        die();
    fclose(random_file);
    srand(seed);

    for (size_t i = 0; i < BODIES_COUNT; i++)
        body_init_random_in_unit_circle(&bodies[i]);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_ASSERT_NO_ERROR;
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow(
        "n-body", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, 0);
    SDL_ASSERT_NO_ERROR;
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_ASSERT_NO_ERROR;
    TTF_Font *font = TTF_OpenFont(font_path, 16);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    struct timespec previous_time;
    clock_gettime(CLOCK_MONOTONIC, &previous_time);
    bool running = true;
    bool paused = false;
    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym)
                {
                case SDLK_q:
                case SDLK_ESCAPE: running = false; break;
                case SDLK_SPACE: paused = !paused; break;
                }
            }
        }
        if (paused)
        {
            SDL_Delay(10);
            continue;
        }

        struct quadtree *bodies_quadtree = quadtree_new();
        bodies_quadtree->start_x = -1.0;
        bodies_quadtree->start_y = -1.0;
        bodies_quadtree->end_x = 2.0;
        bodies_quadtree->end_y = 2.0;
        for (size_t i = 0; i < BODIES_COUNT; i++)
            quadtree_insert(bodies_quadtree, bodies[i]);
        quadtree_update_mass(bodies_quadtree);
        // update_bodies(bodies_quadtree);

        struct worker_arg args[THREAD_POOL_COUNT] = {0};
        size_t            stride = BODIES_COUNT / THREAD_POOL_COUNT;
        for (size_t i = 0, start_index = 0; i < THREAD_POOL_COUNT; i++, start_index += stride)
        {
            args[i] = (struct worker_arg){
                .start_index = start_index,
                .stop_index = start_index + stride,
                .quadtree = bodies_quadtree,
            };
            pthread_create(&thread_pool[i], NULL, (void *(*)(void *))worker_func, &args[i]);
        }
        for (int i = 0; i < THREAD_POOL_COUNT; i++)
            pthread_join(thread_pool[i], NULL);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw_quadtree(bodies_quadtree, 0);
        draw_bodies();
        quadtree_destroy(bodies_quadtree);

        // Compute and fps and display it
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long int time_difference = current_time.tv_nsec - previous_time.tv_nsec;
        time_difference /= 1000000;
        if (time_difference > 0)
        {
            long int fps = 1000 / time_difference;
            char     buf[128] = {0};
            snprintf(buf, 128, "%ld fps", fps);
            SDL_Surface *surface = TTF_RenderText_Solid(font, buf, (SDL_Color){100, 255, 100, 255});
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            SDL_Rect r = {.x = 5, .y = 5};
            SDL_QueryTexture(texture, NULL, NULL, &r.w, &r.h);
            SDL_RenderCopy(renderer, texture, NULL, &r);
            SDL_DestroyTexture(texture);
        }
        memcpy(&previous_time, &current_time, sizeof previous_time);
        SDL_RenderPresent(renderer);
        // SDL_Delay(100);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
