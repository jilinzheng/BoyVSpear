#ifndef SPEAR_BLOCKER_H
#define SPEAR_BLOCKER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Include SDL_ttf for text rendering
#include <vector>
#include <string>
#include "menu.h"
#include "assets.h"
#include <cstdlib> // For rand() and srand()
#include <cmath>   // For M_PI, sin, cos

int SpearBlockerMain(SDL_Window* window, SDL_Renderer* renderer);

namespace spear_blocker {
// --- Enums ---
    enum class GameState {
        MENU,
        PLAYING,
        GAME_OVER
    };

    enum class Difficulty {
        EASY, MEDIUM, HARD
    };

    struct GameSettings {
        int spearSpeed;
        int spawnRate;
        int spearMult;
    };

    // --- Function Prototypes ---
    int HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame);
    void ResetGame(Player& player, std::vector<Spear>& spears, GameState& gameState, const GameSettings& settings);
    void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const GameSettings& settings);
    void SpawnSpear(std::vector<Spear>& spears, const GameSettings& settings);
    void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag);
    bool CheckSpearInBlockZone(const Spear& spear, const SDL_Rect& blockZone);
    GameSettings GetSettingsForDifficulty(Difficulty difficulty);
}

#endif // SPEAR_DODGER_H
