#ifndef CHARACTER_H
#define CHARACTER_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Include SDL_ttf for text rendering

static const int PLAYER_SIZE = 40; 
static const int PLAYER_SPEED = 5;

enum class Direction {
    NONE, UP, DOWN, LEFT, RIGHT
};


struct Player {
    SDL_Rect rect;
    Direction facing;
    float x, y;
};

void RenderPlayerCharacter(SDL_Renderer* renderer, const Player& player, bool isGameOver, int Game_Type);

#endif // CHARACTER_H