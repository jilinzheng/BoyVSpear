#ifndef SPEAR_RUNNER_H
#define SPEAR_RUNNER_H

#include <SDL2/SDL.h>
#include <vector>
#include "assets.h"
#include <SDL2/SDL_ttf.h> // Include SDL_ttf for text rendering
#include <string>
#include "menu.h"
#include <ctime>   // For time()



enum class SpearRunnerGameState {
    MENU,
    PLAYING,
    GAME_OVER
};

struct RunnerSettings {
    int spearSpeed;
    int spawnRate;
};

enum class SpearRunnerDifficulty {
    EASY,
    MEDIUM,
    HARD
};
// --- Function Prototypes ---

int SpearRunnerMain(SDL_Window* window, SDL_Renderer* renderer);
void RenderRunnerMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
#endif // SPEAR_RUNNER_H
