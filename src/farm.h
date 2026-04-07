#ifndef FARM_H
#define FARM_H

#include "tile.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

#define GRID_COLS 10
#define GRID_ROWS 8
#define TILE_SIZE 64

typedef struct {
    Tile tiles[GRID_ROWS][GRID_COLS];
    PlantType plant_types[MAX_PLANT_TYPES];
    int plant_type_count;
    int seed_inventory[MAX_PLANT_TYPES];
    int crop_inventory[MAX_PLANT_TYPES];
    int selected_seed;
    int watering_mode;
    int shop_open;
    int money;
    int day;
} Farm;

void farm_init(Farm *farm);
void farm_update(Farm *farm);
int  farm_harvest(Farm *farm, int row, int col);
void farm_try_breed(Farm *farm, int row, int col);
int  farm_add_plant_type(Farm *farm, PlantType p);
void farm_draw(Farm *farm, SDL_Renderer *renderer);
void farm_draw_ui(Farm *farm, SDL_Renderer *renderer, TTF_Font *font);
void farm_draw_actionbar(Farm *farm, SDL_Renderer *renderer, TTF_Font *font);
void farm_draw_shop(Farm *farm, SDL_Renderer *renderer, TTF_Font *font);
void farm_draw_tooltip(Farm *farm, SDL_Renderer *renderer, TTF_Font *font, int mx, int my);
void farm_handle_click(Farm *farm, int pixel_x, int pixel_y, int button);
void farm_handle_shop_click(Farm *farm, int pixel_x, int pixel_y);
void farm_save(Farm *farm, const char *filename);
void farm_load(Farm *farm, const char *filename);

#endif