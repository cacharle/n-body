#include <stdbool.h>
#include <SDL2/SDL.h>
#include <stdlib.h>

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

struct body {
    double mass;
    double x;
    double y;
    double velocity_x;
    double velocity_y;
};

// SDL2
static const uint32_t window_width = 600;
static const uint32_t window_height = 600;
static SDL_Renderer *renderer = NULL;
// bodies
static const double gravity = 1.0;
#define BODIES_COUNT 3
static struct body bodies[BODIES_COUNT] = {0};

void die(void)
{
    perror("n-body");
    exit(EXIT_FAILURE);
}

double frand()
{
    return (double)rand() / (double)RAND_MAX;
}

double two_bodies_force(struct body b1, struct body b2)
{
    double distance_x = fabs(b1.x - b2.x);
    double distance_y = fabs(b1.y - b2.y);
    double distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    return (b1.mass * b2.mass * gravity) / distance;
}

void draw_bodies(void)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (size_t i = 0; i < BODIES_COUNT; i++)
    {
        uint32_t window_x = bodies[i].x * (double)window_width;
        uint32_t window_y = bodies[i].y * (double)window_height;
        printf("%.2f %.2f %u %u\n",bodies[i].x, bodies[i].y, window_x, window_y);
        SDL_Rect r = {
            .x = window_x,
            .y = window_y,
            .w = 10,
            .h = 10,
        };
        SDL_RenderFillRect(renderer, &r);
    }
}

int main(void)
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
        bodies[i].mass = frand();
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_ASSERT_NO_ERROR;
    SDL_Window *window = SDL_CreateWindow(
        "n-body", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, 0);
    SDL_ASSERT_NO_ERROR;
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_ASSERT_NO_ERROR;

    bool running = true;
    while (running)
    {
        SDL_Event   e;
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
        draw_bodies();
        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
