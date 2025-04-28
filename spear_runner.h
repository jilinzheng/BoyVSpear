#ifndef SPEAR_RUNNER_H
#define SPEAR_RUNNER_H

#include <SDL2/SDL.h>
#include <vector>
#include "Character.h"
#include <ctime>   // For time()

enum class SpearDirection {
    UP, DOWN, LEFT, RIGHT
};

struct Spear {
    SDL_Rect rect;
    SpearDirection direction;
};

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
#endif // SPEAR_RUNNER_H
