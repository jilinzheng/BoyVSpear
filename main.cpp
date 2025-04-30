#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "spear_blocker.h"
#include "assets.h"
#include "spear_runner.h"
#include "assets.h"




int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() == -1) {
        std::cout << "Failed to initialize SDL/TTF: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Game Selector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    TTF_Font* font = TTF_OpenFont(FONT_PATH, 28);

    if (!window || !renderer || !font) {
        std::cout << "Error creating window, renderer, or font." << std::endl;
        return 1;
    }

    bool running = true;
    int selectedGame = 0;

    // joystick thread
    std::thread joystick_thread;

    while (running) {
        // Check if stream is open, if not, try to open it.
        if (!fifo_stream.is_open()) {
            // Optional: Check if the FIFO file exists and is a FIFO before opening
            struct stat stat_buf;
            if (stat(FIFO_PATH, &stat_buf) == 0) {
                if (!S_ISFIFO(stat_buf.st_mode)) {
                    std::cerr << "Error: " << FIFO_PATH << " exists but is not a FIFO." << std::endl;
                    sleep(5); // Wait before retrying
                    continue;
                }
            } else {
                // File doesn't exist yet, wait for Python script to create it
                if (errno == ENOENT) {
                std::cout << "FIFO not found, waiting..." << std::endl;
                } else {
                perror("Error checking FIFO status"); // Other stat error
                }
                sleep(2);
                continue;
            }

            // Open the FIFO for reading [4] [6]
            // This will block until the Python script opens it for writing [6]
            std::cout << "Attempting to open FIFO: " << FIFO_PATH << std::endl;
            fifo_stream.open(FIFO_PATH); // Opens in read mode by default

            if (!fifo_stream.is_open()) {
                std::cerr << "Error opening FIFO: " << FIFO_PATH << ". Retrying..." << std::endl;
                // perror("open"); // Can use perror if using low-level open()
                sleep(2); // Wait before retrying
                continue;
            } else {
                std::cout << "FIFO opened successfully." << std::endl;
                // launch the joystick reading thread
                joystick_thread = std::thread(read_joystick);
            }
        }

        // Render Menu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};

        // Simple menu display
        SDL_Surface* surface = TTF_RenderText_Blended(font, "Spear Blocker", selectedGame == 0 ? yellow : white);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 - 50, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        surface = TTF_RenderText_Blended(font, "Spear Runner", selectedGame == 1 ? yellow : white);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        rect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 + 10, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);

        // Handle events
        // SDL_Event e;
        // while (SDL_PollEvent(&e)) {
        //     if (e.type == SDL_QUIT) {
        //         running = false;
        //     } else if (e.type == SDL_KEYDOWN) {
        //         if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP) {
        //             selectedGame = (selectedGame - 1 + 2) % 2;
        //         } else if (e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN) {
        //             selectedGame = (selectedGame + 1) % 2;
        //         } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
        //             if (selectedGame == 0) {
        //                 if(SpearDodgerMain(window, renderer) == -1)
        //                 {
        //                     printf("exiting");
        //                     running = false;
        //                 }
        //             } else if (selectedGame == 1) {
        //                 if(SpearRunnerMain(window, renderer) == -1)
        //                 {
        //                     running = false;
        //                 }
        //             }
        //         } else if (e.key.keysym.sym == SDLK_ESCAPE) {
        //             running = false;
        //         }
        //     }
        // }

        // joy = read_joystick();
        if (joy.y == UP) selectedGame = (selectedGame-1+2)%2;
        if (joy.y == DOWN) selectedGame = (selectedGame+1)%2;
        if (joy.btn == PRESSED) {
            if (selectedGame==0) {
                if (SpearDodgerMain(window, renderer)) {
                    printf("Exiting");
                    running = false;
                }
            }
            else if (selectedGame==1) {
                if (SpearRunnerMain(window, renderer) == -1) {
                    running = false;
                }
            }
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    if (joystick_thread.joinable()) joystick_thread.join();

    return 0;
}
