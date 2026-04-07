#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // fonts
#include <stdio.h>
#include <windows.h>
#include "farm.h"
// farm.h already includes tile.h, SDL, SDL_ttf, and stdio
// so we don't need to include those again — farm.h pulls them in for us

int main(int argc, char *argv[]) {
    AllocConsole();  // attach a console window so printf works
    // AllocConsole is a Windows API function from windows.h
    // -mwindows detaches stdout by default — this reconnects it
    freopen("CONOUT$", "w", stdout);  // redirect printf output to the console window

    // Initialize SDL2 Video (handles window + rendering)
    // SDL_INIT_VIDEO is a flag — we could also pass SDL_INIT_AUDIO | SDL_INIT_TIMER etc.
    // Returns 0 on success, non-zero on failure
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError()); // %s prints a string
        return 1;   // returning non-zero from main signals failure to the OS
    }

    // Initialize SDL_ttf — the font rendering library
    // This is a separate library from SDL2 itself, so it needs its own init
    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Load a font from the assets folder
    // TTF_Font is an opaque type — like SDL_Window, we only hold a pointer to it
    // 18 is the font size in points
    TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 18);
    if (font == NULL) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create the game window
    // SDL_Window is an opaque type — SDL allocates it internally with malloc and gives us back
    // a pointer (the address). We can't create it ourselves because the compiler doesn't know
    // its internal size (it's an "incomplete type" — typedef struct SDL_Window SDL_Window)
    SDL_Window *window = SDL_CreateWindow(
        "Farming Game",                             // title bar text
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, // center on screen
        800, 600,                                   // width, height in pixels
        0                                           // flags: 0 = plain window
    );
    if (window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create the renderer — this is what actually draws pixels to the window
    // -1 = auto-select the best graphics driver
    // SDL_RENDERER_ACCELERATED = use the GPU instead of CPU for drawing
    // We store it as a pointer for the same reason as SDL_Window — it's an opaque heap type
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);  // clean up the window before quitting
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Stack vs Heap reminder:
    //   Stack: local variables (small, fixed size, gone when function returns)
    //   Heap:  malloc'd memory (large, flexible, lives until freed manually)
    //   SDL_Window and SDL_Renderer live on the HEAP — SDL malloc'd them
    //   We only hold a pointer to where they live
    //   That's why we must call SDL_DestroyWindow/SDL_DestroyRenderer at the end
    //   (SDL's "free" — we tell SDL when we're done, it cleans up)

    // ─────────────────────────────────────────────
    // GAME SETUP
    // ─────────────────────────────────────────────

    // Farm lives on the STACK — we declare it directly, compiler knows its size
    // (unlike SDL types, Farm is our own struct and fully defined)
    Farm farm;
    farm_init(&farm);               // & gives us the address of farm, passed as Farm*
    farm_load(&farm, "savegame.bin"); // attempt to load a previous save
                                      // if no save exists, farm_load just does nothing
    printf("Day %d | Money: $%d\n", farm.day, farm.money);
    // Note: we use . (not ->) here because farm is a direct variable, not a pointer

    int running = 1;
    SDL_Event event;
    int mouse_x = 0, mouse_y = 0;  // current mouse position, updated every frame
    // SDL_GetMouseState fills these with the cursor's current pixel coordinates
    // SDL_Event is a union — a C type that can hold different data shapes
    // depending on event.type, different fields become valid:
    //   event.type == SDL_QUIT        → window X was clicked
    //   event.type == SDL_KEYDOWN     → a key was pressed (event.key.keysym.sym = which key)
    //   event.type == SDL_MOUSEBUTTONDOWN → mouse clicked (event.button.x/y/button)
    //   event.type == SDL_MOUSEWHEEL  → scroll wheel (event.wheel.y = direction)

    // ─────────────────────────────────────────────
    // GAME LOOP — runs every frame until the window is closed
    // Every frame does exactly three things in order:
    //   1. Handle input
    //   2. (State updates happen inside input handlers)
    //   3. Draw
    // ─────────────────────────────────────────────

    while (running) {

        // ── 1. INPUT ──────────────────────────────
        // SDL keeps an internal event queue — a list of everything that happened since last frame
        // SDL_PollEvent grabs one event at a time and puts it into our event variable
        // The inner while loop keeps draining the queue until it's empty
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT) {
                running = 0;    // set flag to exit the game loop after this frame
            }

            // Left or right mouse button clicked
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (farm.shop_open)
                    // Route clicks to shop handler when shop is open
                    farm_handle_shop_click(&farm, event.button.x, event.button.y);
                else
                    farm_handle_click(&farm, event.button.x, event.button.y, event.button.button);
            }

            // Keyboard key pressed
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_s) {
                    farm_save(&farm, "savegame.bin"); // press S to save
                }
            }

            // Scroll wheel — cycle through seed types in the inventory
            if (event.type == SDL_MOUSEWHEEL) {
                // event.wheel.y is positive when scrolling up, negative when scrolling down
                farm.selected_seed += event.wheel.y > 0 ? -1 : 1;
                // The ternary operator: condition ? value_if_true : value_if_false
                // Scrolling up = go to previous seed type, scrolling down = next

                // Wrap around if we go out of bounds (like a circular list)
                if (farm.selected_seed < 0)
                    farm.selected_seed = farm.plant_type_count - 1;
                if (farm.selected_seed >= farm.plant_type_count)
                    farm.selected_seed = 0;
            }
        }

        // ── 2. DRAW ───────────────────────────────
        // Always clear first, draw everything, then present
        // SDL uses double buffering — we draw to a hidden back buffer
        // SDL_RenderPresent swaps it to the screen all at once (prevents flickering)

        // Get current mouse position every frame — used for tooltip
        // SDL_GetMouseState writes x and y directly via pointers
        SDL_GetMouseState(&mouse_x, &mouse_y);

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        if (farm.shop_open) {
            farm_draw_shop(&farm, renderer, font);
        } else {
            farm_draw(&farm, renderer);
            farm_draw_ui(&farm, renderer, font);
            farm_draw_actionbar(&farm, renderer, font);
            // Tooltip drawn last so it appears on top of everything
            farm_draw_tooltip(&farm, renderer, font, mouse_x, mouse_y);
        }

        SDL_RenderPresent(renderer);    // flip back buffer to screen
    }

    // ─────────────────────────────────────────────
    // CLEANUP — free everything in reverse order of creation
    // C has no garbage collector — everything we created, we destroy
    // SDL did malloc internally, so SDL must free it — we just tell it when
    // ─────────────────────────────────────────────
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;   // return 0 from main = success
}

// ─────────────────────────────────────────────
// COMPILE COMMANDS (for reference)
// ─────────────────────────────────────────────

// Basic (no SDL):
//   gcc src/main.c -o farming_game.exe

// With SDL2:
//   gcc src/main.c -o farming_game.exe -I/c/msys64/ucrt64/include/SDL2 -L/c/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -mwindows

// With multiple .c files:
//   gcc src/main.c src/tile.c src/farm.c -o farming_game.exe -I/c/msys64/ucrt64/include/SDL2 -L/c/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -mwindows

// With SDL_ttf (fonts):
//   gcc src/main.c src/tile.c src/farm.c -o farming_game.exe -I/c/msys64/ucrt64/include/SDL2 -L/c/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -mwindows