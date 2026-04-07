#include "farm.h"
#include <string.h>     // for strlen (string length), strncpy (safe string copy), strncat (safe string append)
#include <stdlib.h>     // for rand() (random number), srand() (seed the RNG)
#include <time.h>       // for time() — used to seed the random number generator with the current time

// ─────────────────────────────────────────────
// HELPER FUNCTIONS (private to this file)
// ─────────────────────────────────────────────

// "static" means this function is private to farm.c — no other file can call it
// It's a helper we use internally, like a private method in Python

// get_plant_type — looks up a PlantType by its id
// Returns a pointer to the PlantType, or NULL if not found
// NULL is C's version of None — a pointer that points to nothing
static PlantType *get_plant_type(Farm *farm, int id) {
    for (int i = 0; i < farm->plant_type_count; i++) {
        if (farm->plant_types[i].id == id)
            return &farm->plant_types[i];  // & gives us the address of this plant type
    }
    return NULL;    // not found
}

// ─────────────────────────────────────────────
// FARM SETUP
// ─────────────────────────────────────────────

// farm_add_plant_type — registers a new PlantType, or returns existing index if name already exists
// This prevents duplicates — if "Tomeat" already exists, we just return its index
int farm_add_plant_type(Farm *farm, PlantType p) {
    // First check if a plant with this name already exists
    for (int i = 0; i < farm->plant_type_count; i++) {
        // strcmp compares two strings — returns 0 if they are identical
        if (strcmp(farm->plant_types[i].name, p.name) == 0)
            return i;   // already exists, return its index so caller adds to its inventory
    }

    // Name not found — register it as a new type
    if (farm->plant_type_count >= MAX_PLANT_TYPES) return -1;   // registry full
    int idx = farm->plant_type_count;
    p.id = idx;
    farm->plant_types[idx] = p;
    farm->plant_type_count++;
    return idx;
}

// farm_init — sets up a brand new farm with default values
void farm_init(Farm *farm) {
    // srand seeds the random number generator using the current time
    // If we didn't do this, rand() would produce the same sequence every run
    // time(NULL) returns the current Unix timestamp (seconds since Jan 1 1970)
    // (unsigned int) casts it to the type srand expects
    srand((unsigned int)time(NULL));

    farm->money = 100;
    farm->day = 1;
    farm->plant_type_count = 0; // no plant types registered yet
    farm->selected_seed = 0;
    farm->watering_mode = 0;
    farm->shop_open = 0;    // shop closed by default

    // Clear both inventories
    for (int i = 0; i < MAX_PLANT_TYPES; i++) {
        farm->seed_inventory[i] = 0;
        farm->crop_inventory[i] = 0;
    }

    // Register the 3 starter plant types
    PlantType wheat  = {"Wheat",  240, 210, 80,  25, 20, 0};
    PlantType tomato = {"Tomato", 220, 50,  50,  20, 30, 0};
    PlantType carrot = {"Carrot", 230, 130, 30,  30, 25, 0};
    PlantType blueberry = {"Blueberry", 80,  80,  220, 15, 40, 0}; // slow, very valuable
    PlantType pumpkin   = {"Pumpkin",   230, 120, 20,  20, 35, 0}; // medium speed
    PlantType corn      = {"Corn",      255, 230, 50,  30, 22, 0}; // fast, cheaper

    farm_add_plant_type(farm, wheat);       // id = 0
    farm_add_plant_type(farm, tomato);      // id = 1
    farm_add_plant_type(farm, carrot);      // id = 2
    farm_add_plant_type(farm, blueberry);   // id = 3 — buyable in shop
    farm_add_plant_type(farm, pumpkin);     // id = 4 — buyable in shop
    farm_add_plant_type(farm, corn);        // id = 5 — buyable in shop

    // Give player starting seeds of the 3 starter types only
    // Blueberry, Pumpkin, Corn must be purchased from the shop
    farm->seed_inventory[0] = 5;
    farm->seed_inventory[1] = 5;
    farm->seed_inventory[2] = 5;

    // Initialize every tile in the grid to empty
    for (int i = 0; i < GRID_ROWS; i++)
        for (int j = 0; j < GRID_COLS; j++)
            tile_init(&farm->tiles[i][j]);  // & passes the address of this specific tile
}

// ─────────────────────────────────────────────
// GAME LOGIC
// ─────────────────────────────────────────────

// farm_update — advances the game by one day
// Called when the player clicks Next Day or right-clicks
void farm_update(Farm *farm) {
    farm->day++;

    for (int i = 0; i < GRID_ROWS; i++) {
        for (int j = 0; j < GRID_COLS; j++) {
            Tile *tile = &farm->tiles[i][j];

            // Look up this tile's plant type to get its specific growth rate
            int rate = 25;  // fallback default
            if (tile->plant_type_id >= 0) {
                PlantType *pt = get_plant_type(farm, tile->plant_type_id);
                if (pt != NULL) rate = pt->growth_rate;
            }

            // Remember state before update so we can detect the moment it turns GROWN
            TileState prev_state = tile->state;
            tile_update(tile, rate);

            // If this tile JUST became grown this day AND hasn't bred yet — try breeding
            // prev_state != TILE_GROWN means it wasn't grown before this update
            // tile->state == TILE_GROWN means it just crossed the finish line
            // tile->bred == 0 means it hasn't had its one breeding chance yet
            if (prev_state != TILE_GROWN && tile->state == TILE_GROWN && tile->bred == 0) {
                tile->bred = 1;     // mark as bred so this never fires again for this plant
                farm_try_breed(farm, i, j);  // check neighbours for a breeding chance
            }
        }
    }
}

// farm_harvest — harvests a grown tile
// Gives the player 1-4 seeds and 2-5 crops of that plant type
// Returns 1 on success, 0 if nothing to harvest
int farm_harvest(Farm *farm, int row, int col) {
    Tile *tile = &farm->tiles[row][col];

    if (tile->state == TILE_GROWN) {
        int id = tile->plant_type_id;

        if (id >= 0) {
            // rand() % 4 gives 0-3, +1 shifts to 1-4
            int seeds_gained = (rand() % 4) + 1;
            // rand() % 4 gives 0-3, +2 shifts to 2-5
            int crops_gained = (rand() % 4) + 2;

            farm->seed_inventory[id] += seeds_gained;
            farm->crop_inventory[id] += crops_gained;

            printf("Harvested %s: +%d seeds, +%d crops\n",
                farm->plant_types[id].name, seeds_gained, crops_gained);
        }

        // Reset the tile back to empty
        tile->state = TILE_EMPTY;
        tile->growth = 0;
        tile->plant_type_id = -1;
        return 1;
    }
    return 0;
}

// ─────────────────────────────────────────────
// BREEDING SYSTEM
// ─────────────────────────────────────────────

// breed — creates a hybrid PlantType from two parent PlantTypes
// "const PlantType *a" means a pointer to a PlantType we promise not to modify (read-only)
// new_id is the id we want to assign to the child
static PlantType breed(PlantType *a, PlantType *b, int new_id) {
    PlantType child;    // create an empty PlantType on the stack
    child.id = new_id;

    // Build the hybrid name: first half of parent A + second half of parent B
    // strlen returns the number of characters in a string (not counting the '\0')
    int len_a = strlen(a->name);
    int len_b = strlen(b->name);
    int half_a = len_a / 2;     // integer division rounds down: 5/2 = 2
    int half_b = len_b / 2;

    // strncpy safely copies up to N characters from source to destination
    // We copy the first half_a characters of parent A's name into child.name
    strncpy(child.name, a->name, half_a);
    child.name[half_a] = '\0';  // manually add null terminator so it's a valid C string

    // strncat safely appends up to N characters from source onto destination
    // We append the second half of parent B's name
    // b->name + half_b is pointer arithmetic — it moves the start point forward by half_b characters
    strncat(child.name, b->name + half_b, 31 - half_a);
    child.name[31] = '\0';  // ensure the string never overflows its 32-char buffer

    // Average the RGB colors so the hybrid looks like a mix of its parents
    child.r = (a->r + b->r) / 2;
    child.g = (a->g + b->g) / 2;
    child.b = (a->b + b->b) / 2;

    // Average the stats, with a small hybrid bonus on sell price
    child.growth_rate = (a->growth_rate + b->growth_rate) / 2;
    child.sell_price  = (a->sell_price  + b->sell_price)  / 2 + 5;

    return child;   // return the whole struct by value (C copies it back to the caller)
}

// farm_try_breed — called once when a specific tile (row, col) just became GROWN
// Checks its 4 neighbours — if any are also grown and a different type, roll for a hybrid
void farm_try_breed(Farm *farm, int row, int col) {
    // The 4 neighbour directions: up, down, left, right
    int dr[] = {-1, 1,  0, 0};
    int dc[] = { 0, 0, -1, 1};

    Tile *tile = &farm->tiles[row][col];

    for (int d = 0; d < 4; d++) {
        int nr = row + dr[d];
        int nc = col + dc[d];

        // Skip out of bounds neighbours
        if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;

        Tile *neighbour = &farm->tiles[nr][nc];

        // Neighbour must be grown to participate in breeding
        if (neighbour->state != TILE_GROWN) continue;

        // Get both plant types
        PlantType *pa = get_plant_type(farm, tile->plant_type_id);
        PlantType *pb = get_plant_type(farm, neighbour->plant_type_id);
        if (pa == NULL || pb == NULL) continue;

        // Don't breed a plant with itself
        if (pa->id == pb->id) continue;

        // 20% chance to produce a hybrid seed
        if ((rand() % 100) < 20) {
            PlantType child = breed(pa, pb, farm->plant_type_count);
            int idx = farm_add_plant_type(farm, child);
            if (idx >= 0) {
                farm->seed_inventory[idx]++;
                printf("New hybrid bred: %s (sells for $%d)!\n", child.name, child.sell_price);
            }
            return; // one successful breed per tile is enough — stop checking neighbours
        }
    }
}

// ─────────────────────────────────────────────
// RENDERING
// ─────────────────────────────────────────────

// ─────────────────────────────────────────────
// TILE DRAWING HELPERS
// ─────────────────────────────────────────────

// draw_circle — draws a filled circle using SDL primitives
// SDL has no built-in circle function so we use the midpoint algorithm:
// for each row of pixels within the circle's bounding box, calculate
// how wide the circle is at that height and draw a horizontal line
static void draw_circle(SDL_Renderer *renderer, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        // Pythagoras: x^2 + y^2 = r^2, so x = sqrt(r^2 - y^2)
        int dx = (int)SDL_sqrt((double)(radius * radius - dy * dy));
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

// draw_leaf — draws a simple leaf shape as a filled ellipse rotated by offset
static void draw_leaf(SDL_Renderer *renderer, int cx, int cy, int w, int h) {
    // Draw as a squashed circle (ellipse approximation)
    for (int dy = -h; dy <= h; dy++) {
        // Scale the x extent by w/h ratio to make it ellipse-shaped
        double ratio = (double)w / (double)h;
        int dx = (int)(SDL_sqrt((double)(h * h - dy * dy)) * ratio);
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

// get_plant_visual_type — categorises a plant into a visual family
// 0 = round fruit (tomato, blueberry), 1 = leafy (carrot, pumpkin), 2 = tall stalk (wheat, corn)
static int get_plant_visual_type(const char *name) {
    // Check first letter as a cheap heuristic — good enough for our named plants
    if (name[0] == 'T' || name[0] == 'B') return 0;  // Tomato, Blueberry → round fruit
    if (name[0] == 'C' && name[1] == 'a') return 1;  // Carrot → leafy
    if (name[0] == 'P') return 1;                     // Pumpkin → leafy
    if (name[0] == 'W' || name[0] == 'C') return 2;  // Wheat, Corn → tall stalk
    // Hybrids: use sell_price to pick a type so different hybrids look different
    return 1;   // default to leafy
}

// farm_draw — draws all tiles with state-appropriate visuals
void farm_draw(Farm *farm, SDL_Renderer *renderer) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            Tile *tile = &farm->tiles[r][c];

            // Pixel position of this tile's top-left corner
            int tx = c * TILE_SIZE;
            int ty = r * TILE_SIZE;
            // Center of the tile — useful for drawing centered shapes
            int cx = tx + TILE_SIZE / 2;
            int cy = ty + TILE_SIZE / 2;

            // ── Base tile background ──────────────
            switch (tile->state) {
                case TILE_EMPTY:
                    SDL_SetRenderDrawColor(renderer, 101, 60, 25, 255); // medium brown soil
                    break;
                case TILE_PLANTED:
                    SDL_SetRenderDrawColor(renderer, 80, 45, 15, 255);  // darker soil (disturbed)
                    break;
                case TILE_WATERED:
                    SDL_SetRenderDrawColor(renderer, 65, 38, 12, 255);  // darkest (wet soil)
                    break;
                case TILE_GROWN:
                    SDL_SetRenderDrawColor(renderer, 80, 45, 15, 255);  // dark soil base
                    break;
            }

            SDL_Rect rect = {tx, ty, TILE_SIZE - 2, TILE_SIZE - 2};
            SDL_RenderFillRect(renderer, &rect);

            // ── Empty soil detail ─────────────────
            // Draw subtle cross-hatch lines to give soil texture
            if (tile->state == TILE_EMPTY) {
                SDL_SetRenderDrawColor(renderer, 85, 48, 18, 255);  // slightly lighter lines
                // Horizontal furrow lines spaced 12px apart
                for (int fy = ty + 10; fy < ty + TILE_SIZE - 4; fy += 12)
                    SDL_RenderDrawLine(renderer, tx + 4, fy, tx + TILE_SIZE - 6, fy);
            }

            // ── Watered droplets ──────────────────
            if (tile->state == TILE_WATERED) {
                SDL_SetRenderDrawColor(renderer, 80, 140, 220, 255); // blue droplets
                // Draw 3 small droplet dots scattered across the tile
                draw_circle(renderer, tx + 14, ty + 18, 3);
                draw_circle(renderer, tx + 38, ty + 12, 2);
                draw_circle(renderer, tx + 50, ty + 22, 3);
                draw_circle(renderer, tx + 22, ty + 44, 2);
                draw_circle(renderer, tx + 46, ty + 42, 3);
            }

            // ── Planted sprout ────────────────────
            if (tile->state == TILE_PLANTED || tile->state == TILE_WATERED) {
                // Small brown soil mound at bottom center
                SDL_SetRenderDrawColor(renderer, 120, 75, 30, 255);
                SDL_Rect mound = {cx - 10, ty + TILE_SIZE - 20, 20, 10};
                SDL_RenderFillRect(renderer, &mound);

                // Tiny green sprout stem poking up from mound
                SDL_SetRenderDrawColor(renderer, 80, 160, 60, 255);
                SDL_RenderDrawLine(renderer, cx, ty + TILE_SIZE - 20, cx, ty + TILE_SIZE - 30);

                // Two tiny leaves on the sprout
                SDL_RenderDrawLine(renderer, cx, ty + TILE_SIZE - 26, cx - 5, ty + TILE_SIZE - 30);
                SDL_RenderDrawLine(renderer, cx, ty + TILE_SIZE - 26, cx + 5, ty + TILE_SIZE - 30);
            }

            // ── Fully grown plant ─────────────────
            if (tile->state == TILE_GROWN) {
                PlantType *pt = get_plant_type(farm, tile->plant_type_id);
                int vtype = 0;  // default visual type
                int pr = 80, pg = 180, pb = 60;  // default green color

                if (pt != NULL) {
                    vtype = get_plant_visual_type(pt->name);
                    pr = pt->r; pg = pt->g; pb = pt->b;
                }

                if (vtype == 0) {
                    // ── Round fruit (Tomato, Blueberry) ──
                    // Brown stem
                    SDL_SetRenderDrawColor(renderer, 100, 65, 20, 255);
                    SDL_RenderDrawLine(renderer, cx, ty + TILE_SIZE - 10, cx, cy);

                    // Two green leaves on stem
                    SDL_SetRenderDrawColor(renderer, 60, 140, 40, 255);
                    draw_leaf(renderer, cx - 8, cy + 8, 8, 5);
                    draw_leaf(renderer, cx + 8, cy + 8, 8, 5);

                    // Round fruit at top in plant's color
                    SDL_SetRenderDrawColor(renderer, pr, pg, pb, 255);
                    draw_circle(renderer, cx, cy - 6, 12);

                    // Small highlight dot (lighter version of fruit color)
                    SDL_SetRenderDrawColor(renderer,
                        (pr + 255) / 2, (pg + 255) / 2, (pb + 255) / 2, 255);
                    draw_circle(renderer, cx - 4, cy - 10, 3);

                } else if (vtype == 1) {
                    // ── Leafy plant (Carrot, Pumpkin) ────
                    // Central stem
                    SDL_SetRenderDrawColor(renderer, 100, 65, 20, 255);
                    SDL_RenderDrawLine(renderer, cx, ty + TILE_SIZE - 10, cx, cy + 5);

                    // Three fanning leaves in plant color
                    SDL_SetRenderDrawColor(renderer, pr, pg, pb, 255);
                    draw_leaf(renderer, cx,      cy - 5,  9, 14);  // center leaf
                    draw_leaf(renderer, cx - 12, cy + 2,  7, 12);  // left leaf
                    draw_leaf(renderer, cx + 12, cy + 2,  7, 12);  // right leaf

                    // Leaf vein lines
                    SDL_SetRenderDrawColor(renderer, 60, 120, 30, 255);
                    SDL_RenderDrawLine(renderer, cx, cy + 8, cx, cy - 18);
                    SDL_RenderDrawLine(renderer, cx - 12, cy + 12, cx - 18, cy - 8);
                    SDL_RenderDrawLine(renderer, cx + 12, cy + 12, cx + 18, cy - 8);

                } else {
                    // ── Tall stalk (Wheat, Corn) ──────────
                    // Main thick stalk
                    SDL_SetRenderDrawColor(renderer, 100, 65, 20, 255);
                    SDL_RenderDrawLine(renderer, cx, ty + TILE_SIZE - 8, cx, ty + 8);
                    SDL_RenderDrawLine(renderer, cx + 1, ty + TILE_SIZE - 8, cx + 1, ty + 8);

                    // Alternating side leaves along the stalk
                    SDL_SetRenderDrawColor(renderer, 60, 140, 40, 255);
                    draw_leaf(renderer, cx - 8, cy + 10, 10, 5);   // left low
                    draw_leaf(renderer, cx + 8, cy,      10, 5);   // right mid
                    draw_leaf(renderer, cx - 6, cy - 10, 8,  4);   // left high

                    // Grain head at top in plant's color
                    SDL_SetRenderDrawColor(renderer, pr, pg, pb, 255);
                    SDL_Rect head = {cx - 5, ty + 6, 11, 18};
                    SDL_RenderFillRect(renderer, &head);

                    // Grain texture lines on head
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);
                    for (int gl = ty + 8; gl < ty + 22; gl += 4)
                        SDL_RenderDrawLine(renderer, cx - 4, gl, cx + 4, gl);
                }
            }

            // ── Growth progress bar ───────────────
            if (tile->state != TILE_EMPTY && tile->growth > 0) {
                int bar_max_w = TILE_SIZE - 6;
                int bar_w = (tile->growth * bar_max_w) / 100;

                SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
                SDL_Rect bar_bg = {tx + 3, ty + TILE_SIZE - 8, bar_max_w, 4};
                SDL_RenderFillRect(renderer, &bar_bg);

                SDL_SetRenderDrawColor(renderer, 100, 220, 100, 255);
                SDL_Rect bar_fill = {tx + 3, ty + TILE_SIZE - 8, bar_w, 4};
                SDL_RenderFillRect(renderer, &bar_fill);
            }
        }
    }
}

// draw_text — helper that renders a string to the screen at (x, y)
// const char *text is a pointer to a read-only C string
// SDL_ttf renders text in two steps:
//   1. TTF_RenderText_Solid → creates a Surface (raw pixels in RAM)
//   2. SDL_CreateTextureFromSurface → uploads it to the GPU as a Texture
// We must free the Surface after converting — we don't need it anymore
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y) {
    SDL_Color white = {255, 255, 255, 255};     // RGBA white
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    // SDL_Rect defines where on screen to draw: x, y, width, height
    // surface->w and surface->h give us the exact pixel size of the rendered text
    SDL_Rect dst = {x, y, surface->w, surface->h};

    SDL_FreeSurface(surface);                           // free RAM copy — no longer needed
    SDL_RenderCopy(renderer, texture, NULL, &dst);      // draw texture to screen
    SDL_DestroyTexture(texture);                        // free GPU memory
}

// farm_draw_ui — draws the right side panel showing stats and seed inventory
void farm_draw_ui(Farm *farm, SDL_Renderer *renderer, TTF_Font *font) {
    // Draw dark brown panel background
    SDL_SetRenderDrawColor(renderer, 50, 30, 10, 255);
    SDL_Rect panel = {640, 0, 160, 600};    // x=640 (right of grid), y=0, w=160, h=600
    SDL_RenderFillRect(renderer, &panel);

    // Draw white dividing line between grid and panel
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, 640, 0, 640, 600);

    // char buf[64] is a fixed-size string buffer — a 64 character array
    // sprintf writes a formatted string into buf (like Python's f-string but into a variable)
    // e.g. sprintf(buf, "Day %d", 3) puts "Day 3" into buf
    char buf[64];

    sprintf(buf, "Day %d", farm->day);
    draw_text(renderer, font, buf, 655, 20);

    sprintf(buf, "Money: $%d", farm->money);
    draw_text(renderer, font, buf, 655, 50);

    // Draw seed inventory — one line per plant type that has seeds available
    draw_text(renderer, font, "-- Seeds --", 648, 90);

    for (int i = 0; i < farm->plant_type_count; i++) {
        // Skip plant types with no seeds — no point showing them
        if (farm->seed_inventory[i] <= 0) continue;

        // Highlight the currently selected seed type with a yellow background
        if (i == farm->selected_seed) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);   // yellow
            SDL_Rect highlight = {642, 115 + i * 22, 155, 20};      // position shifts down per type
            SDL_RenderFillRect(renderer, &highlight);
        }

        // Draw the plant name and seed count
        // farm->plant_types[i].name accesses the name field of the i-th plant type
        sprintf(buf, "%s: %d", farm->plant_types[i].name, farm->seed_inventory[i]);
        draw_text(renderer, font, buf, 648, 117 + i * 22);
        // i * 22 spaces each seed line 22 pixels apart vertically
    }
}

// ─────────────────────────────────────────────
// INPUT
// ─────────────────────────────────────────────

// farm_draw_actionbar — draws the bottom strip with Save, Next Day, and Water Mode buttons
// The strip sits below the tile grid: y = GRID_ROWS * TILE_SIZE = 512, height = 88px
void farm_draw_actionbar(Farm *farm, SDL_Renderer *renderer, TTF_Font *font) {
    // Draw dark background for the action bar
    SDL_SetRenderDrawColor(renderer, 30, 20, 10, 255);
    SDL_Rect bar = {0, GRID_ROWS * TILE_SIZE, GRID_COLS * TILE_SIZE, 88};
    SDL_RenderFillRect(renderer, &bar);

    // Draw a top border line to separate it from the grid
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, 0, GRID_ROWS * TILE_SIZE, GRID_COLS * TILE_SIZE, GRID_ROWS * TILE_SIZE);

    SDL_Rect btn_save     = {10,  GRID_ROWS * TILE_SIZE + 19, 130, 50};
    SDL_Rect btn_next_day = {150, GRID_ROWS * TILE_SIZE + 19, 130, 50};
    SDL_Rect btn_water    = {290, GRID_ROWS * TILE_SIZE + 19, 130, 50};
    SDL_Rect btn_shop     = {430, GRID_ROWS * TILE_SIZE + 19, 130, 50};

    // Save button (dark green)
    SDL_SetRenderDrawColor(renderer, 40, 100, 40, 255);
    SDL_RenderFillRect(renderer, &btn_save);
    draw_text(renderer, font, "Save (S)", btn_save.x + 22, btn_save.y + 16);

    // Next Day button (dark blue)
    SDL_SetRenderDrawColor(renderer, 40, 40, 120, 255);
    SDL_RenderFillRect(renderer, &btn_next_day);
    draw_text(renderer, font, "Next Day", btn_next_day.x + 22, btn_next_day.y + 16);

    // Water Mode button — yellow when active
    if (farm->watering_mode)
        SDL_SetRenderDrawColor(renderer, 180, 160, 20, 255);
    else
        SDL_SetRenderDrawColor(renderer, 40, 40, 120, 255);
    SDL_RenderFillRect(renderer, &btn_water);
    draw_text(renderer, font, farm->watering_mode ? "Water: ON" : "Water: OFF",
              btn_water.x + 18, btn_water.y + 16);

    // Shop button (dark gold)
    SDL_SetRenderDrawColor(renderer, 120, 90, 20, 255);
    SDL_RenderFillRect(renderer, &btn_shop);
    draw_text(renderer, font, "Shop", btn_shop.x + 40, btn_shop.y + 16);
}

// ─────────────────────────────────────────────
// SHOP
// ─────────────────────────────────────────────

// Seed buy prices — how much it costs to buy one seed of each type in the shop
// Indexed by plant type id, matching farm->plant_types[]
// Starter types (0-2) can also be bought if player runs out
static int seed_buy_price(PlantType *pt) {
    // Buy price is sell_price * 2 — always costs more to buy than you earn selling
    return pt->sell_price * 2;
}

// farm_draw_shop — draws the full shop screen replacing the game view
void farm_draw_shop(Farm *farm, SDL_Renderer *renderer, TTF_Font *font) {
    // Dark background
    SDL_SetRenderDrawColor(renderer, 15, 15, 25, 255);
    SDL_Rect bg = {0, 0, 800, 600};
    SDL_RenderFillRect(renderer, &bg);

    // Title
    draw_text(renderer, font, "=== SHOP ===", 340, 20);

    char buf[64];
    sprintf(buf, "Money: $%d", farm->money);
    draw_text(renderer, font, buf, 340, 50);

    // ── Left side: SELL crops ─────────────────
    draw_text(renderer, font, "-- Sell Crops --", 30, 90);

    int y = 120;
    for (int i = 0; i < farm->plant_type_count; i++) {
        if (farm->crop_inventory[i] <= 0) continue;  // skip if none to sell

        PlantType *pt = &farm->plant_types[i];

        // Draw sell button (green)
        SDL_SetRenderDrawColor(renderer, 30, 100, 30, 255);
        SDL_Rect btn = {30, y, 60, 24};
        SDL_RenderFillRect(renderer, &btn);
        draw_text(renderer, font, "SELL", 38, y + 4);

        // Crop name, quantity, and price per crop
        sprintf(buf, "%s x%d  ($%d ea)", pt->name, farm->crop_inventory[i], pt->sell_price);
        draw_text(renderer, font, buf, 100, y + 4);

        y += 34;    // move down for next row
    }

    if (y == 120)
        draw_text(renderer, font, "No crops to sell.", 30, 120);

    // ── Right side: BUY seeds ─────────────────
    draw_text(renderer, font, "-- Buy Seeds --", 460, 90);

    y = 120;
    for (int i = 0; i < farm->plant_type_count; i++) {
        PlantType *pt = &farm->plant_types[i];
        int price = seed_buy_price(pt);

        // Dim the button if player can't afford it
        if (farm->money >= price)
            SDL_SetRenderDrawColor(renderer, 30, 30, 120, 255);  // blue = affordable
        else
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);   // grey = too expensive

        SDL_Rect btn = {460, y, 60, 24};
        SDL_RenderFillRect(renderer, &btn);
        draw_text(renderer, font, "BUY", 470, y + 4);

        sprintf(buf, "%s  ($%d)  own: %d", pt->name, price, farm->seed_inventory[i]);
        draw_text(renderer, font, buf, 530, y + 4);

        y += 34;
    }

    // ── Close button ──────────────────────────
    SDL_SetRenderDrawColor(renderer, 140, 30, 30, 255);  // red
    SDL_Rect close_btn = {330, 550, 140, 34};
    SDL_RenderFillRect(renderer, &close_btn);
    draw_text(renderer, font, "Close Shop", 348, 558);
}

// farm_handle_shop_click — handles mouse clicks while the shop is open
void farm_handle_shop_click(Farm *farm, int px, int py) {
    // Close button: x 330-470, y 550-584
    if (px >= 330 && px <= 470 && py >= 550 && py <= 584) {
        farm->shop_open = 0;
        return;
    }

    char buf[64];

    // ── Sell crop buttons (left side) ────────
    // Buttons are at x: 30-90, starting y: 120, spaced 34px apart
    // We need to figure out which visible row was clicked
    int y = 120;
    for (int i = 0; i < farm->plant_type_count; i++) {
        if (farm->crop_inventory[i] <= 0) continue;

        if (px >= 30 && px <= 90 && py >= y && py <= y + 24) {
            // Sell ALL crops of this type at once
            PlantType *pt = &farm->plant_types[i];
            int earned = farm->crop_inventory[i] * pt->sell_price;
            farm->money += earned;
            printf("Sold %d %s crops for $%d\n", farm->crop_inventory[i], pt->name, earned);
            farm->crop_inventory[i] = 0;
            return;
        }
        y += 34;
    }

    // ── Buy seed buttons (right side) ────────
    // Buttons are at x: 460-520, starting y: 120, spaced 34px apart
    y = 120;
    for (int i = 0; i < farm->plant_type_count; i++) {
        PlantType *pt = &farm->plant_types[i];
        int price = seed_buy_price(pt);

        if (px >= 460 && px <= 520 && py >= y && py <= y + 24) {
            if (farm->money >= price) {
                farm->money -= price;
                farm->seed_inventory[i]++;
                printf("Bought 1 %s seed for $%d\n", pt->name, price);
            } else {
                printf("Not enough money to buy %s seed ($%d needed)\n", pt->name, price);
            }
            return;
        }
        y += 34;
    }
}

// farm_draw_tooltip — draws a info popup near the mouse cursor when hovering over a tile
// mx, my are the current mouse pixel coordinates passed in from main.c every frame
void farm_draw_tooltip(Farm *farm, SDL_Renderer *renderer, TTF_Font *font, int mx, int my) {
    // Only show tooltip when hovering over the tile grid
    if (mx < 0 || mx >= GRID_COLS * TILE_SIZE) return;
    if (my < 0 || my >= GRID_ROWS * TILE_SIZE) return;

    int col = mx / TILE_SIZE;
    int row = my / TILE_SIZE;
    Tile *tile = &farm->tiles[row][col];

    // No tooltip for empty tiles
    if (tile->state == TILE_EMPTY) return;

    // Build tooltip lines based on tile state
    char line1[64] = "";    // plant name
    char line2[64] = "";    // growth percentage
    char line3[64] = "";    // state description

    PlantType *pt = get_plant_type(farm, tile->plant_type_id);
    if (pt != NULL)
        sprintf(line1, "%s", pt->name);
    else
        sprintf(line1, "Unknown");

    sprintf(line2, "Growth: %d%%", tile->growth);
    // %% prints a literal % sign — a single % would be interpreted as a format specifier

    switch (tile->state) {
        case TILE_PLANTED: sprintf(line3, "Needs watering");  break;
        case TILE_WATERED: sprintf(line3, "Watered");         break;
        case TILE_GROWN:   sprintf(line3, "Ready to harvest!"); break;
        default: break;
    }

    // Tooltip box dimensions
    int tw = 160;   // tooltip width
    int th = 66;    // tooltip height (3 lines * ~20px + padding)
    int tx = mx + 12;   // offset right of cursor
    int ty = my - 10;   // slightly above cursor

    // Clamp to screen so it doesn't go off the right or bottom edge
    // 800 and 600 are the window dimensions
    if (tx + tw > 800) tx = mx - tw - 4;
    if (ty + th > 600) ty = my - th - 4;

    // Draw dark semi-transparent background box
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 220);
    SDL_Rect box = {tx, ty, tw, th};
    SDL_RenderFillRect(renderer, &box);

    // Draw white border around the box
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &box);
    // SDL_RenderDrawRect draws just the outline, not filled

    // Draw the three text lines inside the box
    draw_text(renderer, font, line1, tx + 8, ty + 6);
    draw_text(renderer, font, line2, tx + 8, ty + 24);
    draw_text(renderer, font, line3, tx + 8, ty + 44);
}

// farm_handle_click — processes a mouse click on either the grid or the action bar
void farm_handle_click(Farm *farm, int pixel_x, int pixel_y, int button) {
    if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT) return;

    // ── Action bar clicks ─────────────────────────
    // The action bar starts at y = GRID_ROWS * TILE_SIZE (512px)
    // Check if the click landed in the bar area and within the grid width
    int bar_y = GRID_ROWS * TILE_SIZE;
    if (pixel_y >= bar_y && pixel_y <= bar_y + 88 && pixel_x < GRID_COLS * TILE_SIZE) {
        // Ranges match button rects exactly: x to x + width
        // Save:     x 10  to 140  ({10,  ..., 130, 50})
        // Next Day: x 150 to 280  ({150, ..., 130, 50})
        // Water:    x 290 to 420  ({290, ..., 130, 50})
        // Shop:     x 430 to 560  ({430, ..., 130, 50})
        if (pixel_x >= 10  && pixel_x <= 140)  farm_save(farm, "savegame.bin");
        if (pixel_x >= 150 && pixel_x <= 280)  farm_update(farm);
        if (pixel_x >= 290 && pixel_x <= 420)  farm->watering_mode = !farm->watering_mode;
        if (pixel_x >= 430 && pixel_x <= 560)  farm->shop_open = 1;
        return;
    }

    // ── Tile grid clicks ──────────────────────────
    // Ignore anything outside the grid (e.g. the right UI panel)
    if (pixel_x >= GRID_COLS * TILE_SIZE || pixel_y >= GRID_ROWS * TILE_SIZE)
        return;

    // Convert pixel position to grid coordinates via integer division
    // e.g. pixel x=193 / 64 = column 3
    int col = pixel_x / TILE_SIZE;
    int row = pixel_y / TILE_SIZE;
    Tile *tile = &farm->tiles[row][col];

    if (button == SDL_BUTTON_LEFT) {
        if (farm->watering_mode) {
            // Watering mode ON — only water planted/watered tiles, ignore empty and grown
            tile_water(tile);
        } else {
            // Normal mode — plant on empty, harvest on grown
            // Watering is disabled in normal mode — use watering mode button for that
            if (tile->state == TILE_EMPTY) {
                int sel = farm->selected_seed;
                if (farm->seed_inventory[sel] > 0) {
                    tile_plant(tile, sel);          // plant selected seed type
                    farm->seed_inventory[sel]--;    // consume one seed
                }
            } else if (tile->state == TILE_GROWN) {
                farm_harvest(farm, row, col);       // harvest a ready crop
            }
            // TILE_PLANTED and TILE_WATERED do nothing in normal mode
            // player must switch to watering mode to water
        }
    }

    if (button == SDL_BUTTON_RIGHT) {
        farm_update(farm);  // right click still advances the day as a shortcut
    }
}

// ─────────────────────────────────────────────
// SAVE / LOAD
// ─────────────────────────────────────────────

// farm_save — writes the entire Farm struct to a binary file
void farm_save(Farm *farm, const char *filename) {
    // fopen opens a file. "wb" = write binary mode
    // FILE *f is a pointer to a file handle (another opaque SDL-style type, but from C stdlib)
    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        printf("Error: could not open save file for writing\n");
        return;
    }

    // fwrite writes raw bytes from memory to the file
    // Arguments: pointer to data, size of one element, number of elements, file
    // sizeof(Farm) asks the compiler exactly how many bytes the Farm struct occupies
    // This writes the entire struct — tiles, plant types, inventory — all at once
    fwrite(farm, sizeof(Farm), 1, f);

    fclose(f);  // always close the file when done — like closing a file in Python
    printf("Game saved.\n");
}

// farm_load — reads a binary file back into the Farm struct
void farm_load(Farm *farm, const char *filename) {
    // "rb" = read binary mode
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        // No save file exists yet — this is normal on first launch
        printf("No save found, starting fresh.\n");
        return;
    }

    // fread is the exact reverse of fwrite
    // It reads bytes from the file directly into the Farm struct in memory
    // This restores everything — tiles, money, day, plant types, inventory
    fread(farm, sizeof(Farm), 1, f);

    fclose(f);
    printf("Game loaded. Day %d\n", farm->day);
}