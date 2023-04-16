#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>
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

struct body
{
    double mass;
    double x;
    double y;
    double velocity_x;
    double velocity_y;
};

// SDL2
static const uint32_t window_width = 600;
static const uint32_t window_height = 600;
static SDL_Renderer  *renderer = NULL;
// bodies
static const double gravity = .000005;
#define BODIES_COUNT 30
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
            double force = two_bodies_force(bodies[i], bodies[j]);
            double dx = bodies[i].x - bodies[j].x;
            double dy = bodies[i].y - bodies[j].y;
            double magnitude = sqrt(dx * dx + dy * dy);
            dx /= magnitude;
            dy /= magnitude;
            dx *= force;
            dy *= force;
            acceleration_x -= dx;
            acceleration_y -= dy;
        }
        // FIXME: should have 2 bodies array because I'm computing the force on
        // already modified bodies rn (small inaccuracy, doesn't show in the
        // visualization)
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
        SDL_Rect r = {
            .x = window_x,
            .y = window_y,
            .w = 30 * bodies[i].mass,
            .h = 30 * bodies[i].mass,
        };
        // SDL_RenderFillRect(renderer, &r);
        aacircleRGBA(
            renderer, window_x, window_y, 30 * bodies[i].mass, 255, 255, 255, 255);
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
        bodies[i].mass = frand();
        // bodies[i].mass = 1.0;
        bodies[i].velocity_x = (frand() - 0.5) / 500;
        bodies[i].velocity_y = (frand() - 0.5) / 500;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_ASSERT_NO_ERROR;
    SDL_Window *window = SDL_CreateWindow("n-body",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          window_width,
                                          window_height,
                                          0);
    SDL_ASSERT_NO_ERROR;
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_ASSERT_NO_ERROR;

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
        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
