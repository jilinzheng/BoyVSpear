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


extern const char* FIFO_PATH;
extern std::ifstream fifo_stream;
extern std::string line;


// --- Function Prototypes ---
void CloseSDL(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font);
int HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame);
void ResetGame(Player& player, std::vector<Spear>& spears, GameState& gameState, const GameSettings& settings);
void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const GameSettings& settings);
void SpawnSpear(std::vector<Spear>& spears, const GameSettings& settings);
void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag);
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font);
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
void RenderSpear(SDL_Renderer* renderer, const Spear& spear);
bool CheckSpearInBlockZone(const Spear& spear, const SDL_Rect& blockZone);
GameSettings GetSettingsForDifficulty(Difficulty difficulty);
int SpearDodgerMain(SDL_Window* window, SDL_Renderer* renderer);


#endif // SPEAR_DODGER_H
