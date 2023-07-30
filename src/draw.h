#ifndef DRAW_H
#define DRAW_H

#include "body.h"
#include "quadtree.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <time.h>

void
draw_init();
void
draw_quit();
void
draw_handle_events(bool *running, bool *paused);
void
draw_update(struct body *bodies, size_t bodies_count, bool mass);

#endif
