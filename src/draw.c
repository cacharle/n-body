#include "draw.h"
#include "utils.h"

static const uint32_t window_width = 1000;
static const uint32_t window_height = 1000;
static SDL_Window    *window = NULL;
static SDL_Renderer  *renderer = NULL;
static TTF_Font      *font = NULL;
static const char    *font_paths[] = {
    "/usr/share/fonts/noto/NotoSansMono-SemiBold.ttf",
    "/System/Library/Fonts/SFNSMono.ttf"
};

#define SDL_ASSERT_NO_ERROR                                                           \
    do                                                                                \
    {                                                                                 \
        if (*SDL_GetError() != '\0')                                                  \
        {                                                                             \
            SDL_Log("[ERROR SDL] %s at %s:%d\n", SDL_GetError(), __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                       \
        }                                                                             \
    } while (false);

static struct timespec previous_time;

static void
draw_bodies(struct body *bodies, size_t bodies_count, bool mass);

void
draw_init()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_ASSERT_NO_ERROR;
    TTF_Init();
    window = SDL_CreateWindow(
        "n-body", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, 0);
    SDL_ASSERT_NO_ERROR;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_ASSERT_NO_ERROR;
    for (size_t i = 0; i < ARRAY_LEN(font_paths); i++)
    {
        font = TTF_OpenFont(font_paths[i], 16);
        if (font != NULL)
            break;
    }
    if (font == NULL)
        die("Cannot open font");
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    clock_gettime(CLOCK_MONOTONIC, &previous_time);
}

void
draw_quit()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}

void
draw_handle_events(bool *running, bool *paused)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_QUIT: *running = false; break;
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym)
            {
            case SDLK_q:
            case SDLK_ESCAPE: *running = false; break;
            case SDLK_SPACE: *paused = !(*paused); break;
            }
        }
    }
}

void
draw_update(struct body *bodies, size_t bodies_count, bool mass)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // draw_quadtree(bodies_quadtree, 0);
    draw_bodies(bodies, bodies_count, mass);

    // Compute FPS and display it
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
}

static SDL_Point *bodies_points = NULL;

static void
draw_bodies(struct body *bodies, size_t bodies_count, bool mass)
{
    if (bodies_points == NULL)
        bodies_points = xmalloc(sizeof(SDL_Point) * bodies_count);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    size_t points_i = 0;
    for (size_t i = 0; i < bodies_count; i++)
    {
        uint32_t canvas_x = bodies[i].x * (double)window_width;
        uint32_t canvas_y = bodies[i].y * (double)window_height;
        if (!mass)
        {
            if (canvas_x < 0 || canvas_x > window_width || canvas_y < 0 || canvas_y > window_height)
                continue;
            bodies_points[points_i].x = canvas_x;
            bodies_points[points_i].y = canvas_y;
            points_i++;
        }
        else
        {
            uint32_t radius = 30 * bodies[i].mass;
            aacircleRGBA(renderer, canvas_x, canvas_y, radius, 255, 255, 255, 255);
        }
    }
    if (!mass)
        SDL_RenderDrawPoints(renderer, bodies_points, points_i);
}

// void
// draw_quadtree(struct quadtree *quadtree, unsigned int depth)
// {
//     // if (depth > 8)
//     //     return;
//     uint32_t canvas_start_x = quadtree->start_x * (double)window_width;
//     uint32_t canvas_start_y = quadtree->start_y * (double)window_height;
//     uint32_t canvas_end_x = quadtree->end_x * (double)window_width;
//     uint32_t canvas_end_y = quadtree->end_y * (double)window_height;
//     // uint32_t canvas_center_of_mass_x = quadtree->center_of_mass_x * (double)window_width;
//     // uint32_t canvas_center_of_mass_y = quadtree->center_of_mass_y * (double)window_height;
//     SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
//     SDL_RenderDrawLine(renderer, canvas_start_x, canvas_start_y, canvas_end_x, canvas_start_y);
//     SDL_RenderDrawLine(renderer, canvas_start_x, canvas_start_y, canvas_start_x, canvas_end_y);
//     // if (quadtree->type == QUADTREE_EMPTY)
//     // {
//     //     SDL_SetRenderDrawColor(renderer, 20, 20, 20, 0);
//     //     SDL_RenderFillRect(
//     //         renderer,
//     //         &(SDL_Rect){
//     //             .x = canvas_start_x + 1,
//     //             .y = canvas_start_y + 1,
//     //             .w = canvas_end_x - canvas_start_x - 1,
//     //             .h = canvas_end_y - canvas_start_y - 1
//     //         }
//     //     );
//     // }
//     if (quadtree->type == QUADTREE_INTERNAL)
//     {
//         // filledCircleColor(
//         //     renderer, canvas_center_of_mass_x, canvas_center_of_mass_y, 3, 0xff0000ff);
//         draw_quadtree(quadtree->children.nw, depth + 1);
//         draw_quadtree(quadtree->children.ne, depth + 1);
//         draw_quadtree(quadtree->children.sw, depth + 1);
//         draw_quadtree(quadtree->children.se, depth + 1);
//     }
// }
