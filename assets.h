#ifndef ASSETS_H
#define ASSETS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // include SDL_ttf for text rendering

static const int PLAYER_SIZE = 40; 
static const int PLAYER_SPEED = 5;

// for spears
const int SPEAR_BASE_WIDTH = 5;
const int SPEAR_LENGTH = 20;

enum class Direction {
    NONE, UP, DOWN, LEFT, RIGHT
};

struct Player {
    SDL_Rect rect;
    Direction facing;
    float x, y;
};

struct Spear {
    SDL_Rect rect;
    Direction originDirection;
    float x, y;
    int speed;
};

void RenderPlayerCharacter(SDL_Renderer* renderer, const Player& player, bool isGameOver, int Game_Type);
void RenderSpear(SDL_Renderer* renderer, const Spear& spear);

#endif // ASSETS_H
