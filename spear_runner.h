#ifndef SPEAR_RUNNER_H
#define SPEAR_RUNNER_H

#include <SDL2/SDL.h>
#include <vector>
#include "assets.h"
#include <SDL2/SDL_ttf.h> // Include SDL_ttf for text rendering
#include <string>
#include "menu.h"
#include <ctime>   // For time()

int SpearRunnerMain(SDL_Window* window, SDL_Renderer* renderer);

namespace spear_runner
    {
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
    // --- Function Prototypes ---
    Settings GetSettingsForDifficulty(Difficulty difficulty);
    void HandleInput(Player& player, GameState& gameState, int& selectedOption, bool& gameOver, float& moveX, float& moveY);
    void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag);
    void SpawnSpears(std::vector<Spear>& spears, const Settings& settings);
    void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const Settings& settings, GameState& gameState, int& frameCount);
}

#endif // SPEAR_RUNNER_H
