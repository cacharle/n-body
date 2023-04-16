#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define SDL_ASSERT_NO_ERROR                                                       \
    do                                                                            \
    {                                                                             \
        if (*SDL_GetError() != '\0')                                              \
        {                                                                         \
            SDL_Log(                                                              \
                "[ERROR SDL] %s at %s:%d\n", SDL_GetError(), __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                   \
        }                                                                         \
    } while (false);

struct body
{
    double mass;
    double x;
    double y;
    double velocity_x;
    double velocity_y;
};

// SDL2
static const uint32_t window_width = 1000;
static const uint32_t window_height = 1000;
static SDL_Renderer  *renderer = NULL;
static const char    *font_path = "/usr/share/fonts/noto/NotoSansMono-SemiBold.ttf";
// bodies
static const double gravity = .000002;
#define BODIES_COUNT 2250
static struct body bodies_previous[BODIES_COUNT] = {0};
static struct body bodies[BODIES_COUNT] = {0};

void
die(void)
{
    perror("n-body");
    exit(EXIT_FAILURE);
}

double
frand()
{
    return (double)rand() / (double)RAND_MAX;
}

double
two_bodies_force(struct body b1, struct body b2)
{
    double distance_x = b1.x - b2.x;
    double distance_y = b1.y - b2.y;
    double distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    return (b1.mass * b2.mass * gravity) / distance;
}

void
update_bodies(void)
{
    memcpy(bodies_previous, bodies, sizeof bodies);
    for (size_t i = 0; i < BODIES_COUNT; i++)
    {
        bodies[i].x += bodies[i].velocity_x;
        bodies[i].y += bodies[i].velocity_y;
        // Compute the attraction force of every other bodies to this body (and
        // accumulate it in acceleration)
        double acceleration_x = 0.0;
        double acceleration_y = 0.0;
        for (size_t j = 0; j < BODIES_COUNT; j++)
        {
            if (j == i)
                continue;
            double force = two_bodies_force(bodies_previous[i], bodies_previous[j]);
            double dx = bodies_previous[i].x - bodies_previous[j].x;
            double dy = bodies_previous[i].y - bodies_previous[j].y;
            double magnitude = sqrt(dx * dx + dy * dy);
            dx /= magnitude;
            dy /= magnitude;
            dx *= force;
            dy *= force;
            acceleration_x -= dx;
            acceleration_y -= dy;
        }
        bodies[i].velocity_x += acceleration_x;
        bodies[i].velocity_y += acceleration_y;
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
        aacircleRGBA(renderer, window_x, window_y, radius, 255, 255, 255, 255);
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
    {
        bodies[i].x = frand();
        bodies[i].y = frand();
        bodies[i].mass = frand() + 0.3;
        bodies[i].velocity_x = (frand() - 0.5) / 500;
        bodies[i].velocity_y = (frand() - 0.5) / 500;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_ASSERT_NO_ERROR;
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow("n-body",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          window_width,
                                          window_height,
                                          0);
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
        draw_bodies();

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
            SDL_Surface *surface =
                TTF_RenderText_Solid(font, buf, (SDL_Color){100, 255, 100, 255});
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            SDL_Rect r = {.x = 5, .y = 5};
            SDL_QueryTexture(texture, NULL, NULL, &r.w, &r.h);
            SDL_RenderCopy(renderer, texture, NULL, &r);
            SDL_DestroyTexture(texture);
        }
        memcpy(&previous_time, &current_time, sizeof previous_time);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
