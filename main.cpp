#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "spear_blocker.h"
#include "assets.h"
#include "spear_runner.h"
#include "assets.h"

// Screen size is now 277

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

    while (running) {
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
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP) {
                    selectedGame = (selectedGame - 1 + 2) % 2;
                } else if (e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN) {
                    selectedGame = (selectedGame + 1) % 2;
                } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
                    if (selectedGame == 0) {
                        if(SpearDodgerMain(window, renderer) == -1)
                        {
                            printf("exiting");
                            running = false;
                        }
                        
                    } else if (selectedGame == 1) {
                        if(SpearRunnerMain(window, renderer) == -1)
                        {
                            running = false;
                        }
                    }
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
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

    return 0;
}
