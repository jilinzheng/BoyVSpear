#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "assets.h"
#include <ctime>   // For time()
#include <iostream> // For cout
#include <fstream>  // For ifstream (reading files/FIFOs)
#include <string>   // For string manipulation
#include <unistd.h> // For sleep (optional)
#include <sys/stat.h> // For checking file type (optional but good)
#include <sys/types.h>
#include <fcntl.h> // For low-level open (alternative)
#include <cerrno>   // For errno
#include <cstdio>  // For perror
#include <sstream>


enum CMD {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    NEUTRAL
};

enum BTN_STATE {
    PRESSED,  // software pullup resistor, pressing button gives LOW/0
    RELEASED
};

struct joystick {
    int x, y, btn;
};

// FIFO to read BLE values written by Python BLE client
static const char* FIFO_PATH = "/tmp/joystick_fifo";
static std::ifstream fifo_stream;
static std::string line;

static const int SCREEN_WIDTH = 300;
static const int SCREEN_HEIGHT = 300;
static const char* FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";


joystick read_joystick();
// Function to render text on the screen
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
// Function to render the main menu
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
// Function to render the game over screen
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font, int score);
// Function to render the score
void RenderScore(SDL_Renderer* renderer, TTF_Font* font, int score);


#endif 