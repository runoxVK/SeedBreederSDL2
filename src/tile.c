#include "tile.h"
// Includes our own header so this file knows what Tile, TileState, and PlantType are
// "tile.h" uses quotes (not <>) because it's our own file, not a system library

// tile_init — resets a tile to its default empty state
// Tile *tile is a pointer — it holds the memory address of a Tile
// We use -> to access fields through a pointer (shorthand for (*tile).field)
// If we used . instead of ->, we'd need to dereference first: (*tile).state
void tile_init(Tile *tile) {
    tile->state = TILE_EMPTY;   // set state to the first enum value (0)
    tile->growth = 0;           // no growth yet
    tile->watered = 0;          // not watered
    tile->water_count = 0;      // never been watered
    tile->plant_type_id = -1;   // -1 is our sentinel for "no plant" (like None in Python)
    tile->bred = 0;             // hasn't bred yet
}

// tile_plant — plants a seed of a specific type on this tile
// plant_type_id is the index into farm->plant_types[] that tells us which plant this is
void tile_plant(Tile *tile, int plant_type_id) {
    // Only plant if the tile is empty — ignore clicks on already planted tiles
    if (tile->state == TILE_EMPTY) {
        tile->state = TILE_PLANTED;
        tile->growth = 0;                       // reset growth just in case
        tile->plant_type_id = plant_type_id;    // remember which plant type this is
        tile->bred = 0;                         // reset bred flag for new plant
    }
}

// tile_water — waters a planted or already-watered tile
void tile_water(Tile *tile) {
    // || means OR — water works on both PLANTED and WATERED states
    if (tile->state == TILE_PLANTED || tile->state == TILE_WATERED) {
        tile->watered = 1;          // flag it as watered for this day
        tile->water_count++;        // ++ increments by 1 (same as water_count = water_count + 1)
        tile->state = TILE_WATERED; // update state
    }
}

// tile_update — called once per game day to advance the tile's growth
// growth_rate comes from the PlantType — different plants grow at different speeds
void tile_update(Tile *tile, int growth_rate) {
    // Only grow if the tile was watered — unwatered tiles do nothing
    if (tile->state == TILE_WATERED) {
        tile->growth += growth_rate;    // += is shorthand for growth = growth + growth_rate
        tile->watered = 0;              // reset watered flag — needs watering again tomorrow
        tile->state = TILE_PLANTED;     // go back to needing water

        // Check if fully grown
        // >= means "greater than or equal to"
        if (tile->growth >= 100) {
            tile->state = TILE_GROWN;   // ready to harvest!
        }
    }
}