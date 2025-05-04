#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "assets.h"
#include <ctime>        // for time()
#include <iostream>     // for cout
#include <fstream>      // for ifstream (reading files/FIFOs)
#include <string>       // for string manipulation
#include <unistd.h>     // for sleep (optional)
#include <sys/stat.h>   // for checking file type (optional but good)
#include <sys/types.h>
#include <fcntl.h>      // for low-level open (alternative)
#include <cerrno>       // for errno
#include <cstdio>       // for perror
#include <sstream>
#include <thread>
#include <mutex>

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

extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const char* FONT_PATH;
// FIFO to read BLE values written by Python BLE client
extern const char* FIFO_PATH;
extern std::ifstream fifo_stream;
extern std::string line;
extern Joystick joy;
extern bool joy_action;
extern std::mutex joy_mutex;

void read_joystick();
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font, int score);
void RenderScore(SDL_Renderer* renderer, TTF_Font* font, int score);

#endif 
