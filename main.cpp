#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "assets.h"
#include "spear_blocker.h"
#include "spear_runner.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() == -1) {
        std::cout << "Failed to initialize SDL/TTF: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Game Selector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    TTF_Font* font = TTF_OpenFont(FONT_PATH, 28);

    if (!window || !renderer || !font) {
        std::cout << "Error creating window, renderer, or font." << "\n";
        return 1;
    }

    bool running = true;
    int selectedGame = 0;

    // joystick thread
    std::thread joystick_thread;

    while (running) {
        // check if stream is open, if not, try to open it
        if (!fifo_stream.is_open()) {
            // check if the FIFO file exists and is a FIFO before opening
            struct stat stat_buf;
            if (stat(FIFO_PATH, &stat_buf) == 0) {
                if (!S_ISFIFO(stat_buf.st_mode)) {
                    std::cerr << "Error: " << FIFO_PATH << " exists but is not a FIFO." << "\n";
                    // wait before retrying
                    sleep(5); 
                    continue;
                }
            }
            else {
                // file doesn't exist yet, wait for Python script to create it
                if (errno == ENOENT) std::cout << "FIFO not found, waiting..." << "\n";
                // other stat error
                else perror("Error checking FIFO status");
                sleep(2);
                continue;
            }

            // open the FIFO for reading
            // blocks until the Python script opens FIFO for writing
            std::cout << "Attempting to open FIFO: " << FIFO_PATH << "\n";
            fifo_stream.open(FIFO_PATH);

            if (!fifo_stream.is_open()) {
                std::cerr << "Error opening FIFO: " << FIFO_PATH << ". Retrying..." << "\n";
                sleep(2); // wait before retrying
                continue;
            } else {
                std::cout << "FIFO opened successfully." << "\n";
                // launch the joystick reading thread
                joystick_thread = std::thread(read_joystick);
            }
        }

        // render menu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};

        // simple menu display
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

        bool descend_menu = false, ascend_menu = false, enter_game = false;
        {   // scope the mutex
            std::lock_guard<std::mutex> lock(joy_mutex);
            if (joy_action) {
                joy_action = false;
                if (joy.y == UP) ascend_menu = true;
                else if (joy.y == DOWN) descend_menu = true;
                else if (joy.btn == PRESSED) enter_game = true;
            }
        }   // automatically frees here
        if (ascend_menu) selectedGame = (selectedGame-1+2)%2;
        else if (descend_menu) selectedGame = (selectedGame+1)%2;
        else if (enter_game) {
            if (selectedGame==0) {
                if (SpearBlockerMain(window, renderer) == -1) {
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
