#ifndef SPEAR_BLOCKER_H
#define SPEAR_BLOCKER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>   // include SDL_ttf for text rendering
#include <vector>
#include <string>
#include "menu.h"
#include "assets.h"
#include <cstdlib>          // for rand() and srand()
#include <cmath>            // for M_PI, sin, cos

int SpearBlockerMain(SDL_Window* window, SDL_Renderer* renderer);

namespace spear_blocker {
    inline int RETURN_TO_MENU; // flag to return to menu
    enum class GameState {
        MENU,
        PLAYING,
        GAME_OVER
    };

    enum class Difficulty {
        EASY, MEDIUM, HARD
    };

    struct Settings {
        int spearSpeed;
        int spawnRate;
        int spearMult;
    };

    int HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame);
    void ResetGame(Player& player, std::vector<Spear>& spears, GameState& gameState, const Settings& settings);
    void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const Settings& settings);
    void SpawnSpear(std::vector<Spear>& spears, const Settings& settings);
    void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag);
    bool CheckSpearInBlockZone(const Spear& spear, const SDL_Rect& blockZone);
    Settings GetSettingsForDifficulty(Difficulty difficulty);
}

#endif
