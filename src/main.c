#include "body.h"
#include "quadtree.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
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
#define BODIES_COUNT 2000
static struct body bodies_previous[BODIES_COUNT] = {0};
static struct body bodies[BODIES_COUNT] = {0};

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
        body_gravitational_force(*body, bodies_previous[j], &force_x, &force_y);
        acceleration_x_ -= force_x;
        acceleration_y_ -= force_y;
    }
    body->velocity_y += acceleration_y_;
    body->velocity_x += acceleration_x_;
}

void
update_bodies(void)
{
    memcpy(bodies_previous, bodies, sizeof bodies);
    for (size_t i = 0; i < BODIES_COUNT; i++)
    {
        body_acceleration(&bodies[i]);
        bodies[i].x += bodies[i].velocity_x;
        bodies[i].y += bodies[i].velocity_y;
    }
}

void
draw_bodies(void)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (size_t i = 0; i < BODIES_COUNT; i++)
    {
        uint32_t window_x = bodies[i].x * (double)window_width;
        uint32_t window_y = bodies[i].y * (double)window_height;
        uint32_t radius = 30 * bodies[i].mass;
        SDL_RenderDrawPoint(renderer, window_x, window_y);
        // aacircleRGBA(renderer, window_x, window_y, radius, 255, 255, 255, 255);
    }
}

int
main(void)
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
        body_init_random(&bodies[i]);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_ASSERT_NO_ERROR;
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow(
        "n-body", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, 0);
    SDL_ASSERT_NO_ERROR;
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_ASSERT_NO_ERROR;
    TTF_Font *font = TTF_OpenFont(font_path, 16);

    struct timespec previous_time;
    clock_gettime(CLOCK_MONOTONIC, &previous_time);
    bool running = true;
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
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        update_bodies();

        // struct quadtree *bodies_quadtree = quadtree_new();
        // bodies_quadtree->start_x = -100.0;
        // bodies_quadtree->start_y = -100.0;
        // bodies_quadtree->end_x = 100.0;
        // bodies_quadtree->end_y = 100.0;
        // for (size_t i = 0; i < BODIES_COUNT; i++)
        //     quadtree_insert(bodies_quadtree, bodies[i]);
        // quadtree_update_mass(bodies_quadtree);
        //
        draw_bodies();

        // quadtree_destroy(bodies_quadtree);

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
        // SDL_Delay(10);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
