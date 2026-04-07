#ifndef TILE_H
// Header Guard: if this file has already been included somewhere, skip it entirely
// This prevents the compiler from seeing the same definitions twice, which causes errors
// The name TILE_H is just a convention — named after the file in caps
#define TILE_H

// How many different plant types can exist in total (starter plants + any bred hybrids)
// #define is a preprocessor constant — before compiling, every instance of MAX_PLANT_TYPES
// gets replaced with 64. Like a find-and-replace before the compiler even runs.
#define MAX_PLANT_TYPES 64

// PlantType — defines the identity of a seed/crop
// typedef struct lets us write "PlantType" instead of "struct PlantType" everywhere
// Think of it like a class in Python but with ONLY data, no methods
typedef struct {
    char name[32];      // the plant's name — char[] is a C string (array of characters)
                        // [32] means max 31 characters + a null terminator '\0' at the end
                        // C strings always end with '\0' so the computer knows where they stop
    int r, g, b;        // RGB color values (0-255 each) — what color this crop shows when grown
    int growth_rate;    // how much growth is added each watered day (Wheat=25, Tomato=20 etc.)
    int sell_price;     // money earned when this crop is harvested
    int id;             // unique number that identifies this plant type
                        // used to look up a PlantType from a tile's plant_type_id field
} PlantType;

// TileState — what state a tile can be in
// typedef enum creates a named set of integer constants
// secretly: TILE_EMPTY=0, TILE_PLANTED=1, TILE_WATERED=2, TILE_GROWN=3
// using names instead of numbers makes the code readable
typedef enum {
    TILE_EMPTY,         // bare soil, nothing planted
    TILE_PLANTED,       // seed in ground, needs watering
    TILE_WATERED,       // watered, will grow when day advances
    TILE_GROWN,         // fully grown, ready to harvest
} TileState;

// Tile — represents one square on the farm grid
typedef struct {
    TileState state;    // current state (EMPTY, PLANTED, WATERED, or GROWN)
    int growth;         // 0 to 100 — how grown the plant is
    int watered;        // 1 if watered today, 0 if not (like a boolean — C has no bool by default)
    int water_count;    // total number of times this tile has been watered (tracked for fun)
    int plant_type_id;  // which PlantType is planted here
                        // -1 means nothing is planted (we use -1 as a "null" sentinel value)
    int bred;           // 1 if this tile has already triggered breeding, 0 if not
                        // prevents a grown tile from breeding multiple times
} Tile;

// Function declarations — these tell other files "these functions exist and here's their signature"
// The actual code (the definition) lives in tile.c
// Tile *tile means "a pointer to a Tile" — we pass the address so the function can modify it
// (if we passed Tile directly, C would copy it and changes wouldn't affect the original)
void tile_init(Tile *tile);                         // reset a tile to empty/zero
void tile_plant(Tile *tile, int plant_type_id);     // plant a seed of a given type
void tile_water(Tile *tile);                        // water a planted tile
void tile_update(Tile *tile, int growth_rate);      // advance growth by one day

#endif
// End of header guard — everything between #ifndef and #endif is protected from double-inclusion