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
#include <thread>


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

struct Joystick {
    int x, y, btn;

    Joystick& operator=(const Joystick& other) {
        if (this == &other) {
            return *this; // Return reference to self
        }
        this->x = other.x;
        this->y = other.y;
        this->btn = other.btn;
        return *this;
    }

    bool operator==(const Joystick& other) const {
        return this->x == other.x &&
                this->y == other.y &&
                this->btn == other.btn;
    }

    bool operator!=(const Joystick& other) const {
        return !(*this == other); // Often implemented using ==
    }
};

// FIFO to read BLE values written by Python BLE client
extern const char* FIFO_PATH;
extern std::ifstream fifo_stream;
extern std::string line;
extern Joystick joy;
extern bool joy_action;

static const int SCREEN_WIDTH = 300;
static const int SCREEN_HEIGHT = 300;
static const char* FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";


void read_joystick();
// Function to render text on the screen
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
// Function to render the main menu
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
// Function to render the game over screen
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font, int score);
// Function to render the score
void RenderScore(SDL_Renderer* renderer, TTF_Font* font, int score);


#endif 