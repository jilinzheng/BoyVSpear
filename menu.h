#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "assets.h"

static const int SCREEN_WIDTH = 277;
static const int SCREEN_HEIGHT = 277;
static const char* FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; // CHANGE THIS to a valid TTF font path!

// Function to render text on the screen
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
// Function to render the main menu
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
// Function to render the game over screen
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font, int score);
// Function to render the score
void RenderScore(SDL_Renderer* renderer, TTF_Font* font, int score);


#endif 