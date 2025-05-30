#ifndef SPEAR_RUNNER_H
#define SPEAR_RUNNER_H

#include <SDL2/SDL.h>
#include <vector>
#include "assets.h"
#include <SDL2/SDL_ttf.h>   // include SDL_ttf for text rendering
#include <string>
#include "menu.h"
#include <ctime>            // for time()

int SpearRunnerMain(SDL_Window* window, SDL_Renderer* renderer);

namespace spear_runner {
    inline int RETURN_TO_MENU; // flag to return to menu
    enum class GameState {
        MENU,
        PLAYING,
        GAME_OVER
    };

    struct Settings {
        int spearSpeed;
        int spawnRate;
    };

    enum class Difficulty {
        EASY,
        MEDIUM,
        HARD
    };

    Settings GetSettingsForDifficulty(Difficulty difficulty);
    int HandleInput(Player& player, GameState& gameState, int& selectedOption, bool& gameOver, \
                    float& moveX, float& moveY, Settings settings, int& frameCount, std::vector<Spear>& spears);
    void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag);
    void SpawnSpears(std::vector<Spear>& spears, const Settings& settings);
    void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const Settings& settings, GameState& gameState, int& frameCount, float moveX, float moveY);
}

#endif
