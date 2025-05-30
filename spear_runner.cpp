#include "spear_runner.h"
#include "assets.h"
#include <cstdlib>
#include <ctime>

using namespace spear_runner;
int SCORE_TIMER = 0;
Uint32 lastIncrementTime = SDL_GetTicks();  // current time in milliseconds

int SpearRunnerMain(SDL_Window* window, SDL_Renderer* renderer) {
    TTF_Font* font = nullptr;
    font = TTF_OpenFont(FONT_PATH, 28);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        printf("Ensure the path '%s' is correct.\n", FONT_PATH);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return false;
    }

    srand(time(0));

    GameState gameState = GameState::MENU;
    int selectedOption = 1; // 0=Easy, 1=Medium, 2=Hard, 3=Back
    bool gameOver = false;
    int frameCount = 0;
    Settings settings = GetSettingsForDifficulty(Difficulty::MEDIUM);

    Player player;
    player.x = SCREEN_WIDTH / 2.0f;
    player.y = SCREEN_HEIGHT / 2.0f;
    player.rect.w = PLAYER_SIZE;
    player.rect.h = PLAYER_SIZE;
    player.rect.x = static_cast<int>(player.x - player.rect.w / 2);
    player.rect.y = static_cast<int>(player.y - player.rect.h / 2);
    player.facing = Direction::UP;

    std::vector<Spear> spears;

    while (true) {
        printFPS();

        float moveX = 0, moveY = 0;

        if (HandleInput(player, gameState, selectedOption, gameOver, moveX, moveY, settings, frameCount, spears) == -1) {
            return -1;
        }

        if (gameState == GameState::MENU) {
            if (RETURN_TO_MENU) {
                RETURN_TO_MENU = 0;
                return 0; // exit the game loop
            }
        }

        // gameplay logic
        if (gameState == GameState::PLAYING && !gameOver) {
            // update score
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime > lastIncrementTime + 1000) { // 1000 ms = 1 second
                SCORE_TIMER++;
                lastIncrementTime = currentTime;
            }
            UpdateGame(player, spears, gameOver, settings, gameState, frameCount, moveX, moveY);
        }
        RenderGame(renderer, font, player, spears, gameState, selectedOption, gameOver);
    }
    return 0;
}

namespace spear_runner {
    Settings GetSettingsForDifficulty(Difficulty difficulty) {
        Settings settings;
        switch (difficulty) {
            case Difficulty::EASY: settings.spearSpeed = 2; settings.spawnRate = 60; break;
            case Difficulty::MEDIUM: settings.spearSpeed = 4; settings.spawnRate = 40; break;
            case Difficulty::HARD: settings.spearSpeed = 6; settings.spawnRate = 20; break;
            default: settings.spearSpeed = 4; settings.spawnRate = 40; break;
        }
        return settings;
    }

    int HandleInput(Player& player, GameState& gameState, int& selectedOption, bool& gameOver, \
                    float& moveX, float& moveY, Settings settings, int& frameCount, std::vector<Spear>& spears) {
        if (gameState == GameState::MENU) {
            std::lock_guard<std::mutex> lock(joy_mutex);
            if (joy_action) {
                joy_action = false;
                if (joy.y == UP) selectedOption = (selectedOption - 1 + 4) % 4;
                else if (joy.y == DOWN) selectedOption = (selectedOption + 1) % 4;
                else if (joy.btn == PRESSED) {
                    if (selectedOption == 3) {
                        RETURN_TO_MENU = 1; // back selected
                        return 0;           // back selected
                    }
                    else {
                        settings = GetSettingsForDifficulty(static_cast<Difficulty>(selectedOption));
                        gameOver = false;
                        frameCount = 0;
                        spears.clear();
                        player.x = SCREEN_WIDTH / 2.0f;
                        player.y = SCREEN_HEIGHT / 2.0f;
                        player.rect.x = static_cast<int>(player.x - player.rect.w / 2);
                        player.rect.y = static_cast<int>(player.y - player.rect.h / 2);
                        gameState = GameState::PLAYING;
                    }
                }
            }
        }
        else if (gameState == GameState::GAME_OVER) {
            std::lock_guard<std::mutex> lock(joy_mutex);
            if (joy_action) {
                joy_action = false;
                if (joy.btn==PRESSED)
                    gameState = GameState::MENU;
            }
        }

        if (gameState == GameState::PLAYING && !gameOver) {
            if (joy.y == UP) moveY = -PLAYER_SPEED;
            if (joy.y == DOWN) moveY = PLAYER_SPEED;
            if (joy.x == LEFT) moveX = -PLAYER_SPEED;
            if (joy.x == RIGHT) moveX = PLAYER_SPEED;
        }

        return 0;
    }

    void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState == GameState::MENU) {
            SCORE_TIMER = 0;
            RenderMenu(renderer, font, selectedOption);
        }
        else {
            RenderPlayerCharacter(renderer, player, gameOverFlag, 0);
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            RenderScore(renderer, font, SCORE_TIMER); // render score

            for (auto& spear : spears) {
                RenderSpear(renderer, spear); // draw spears
            }

            if (gameState == GameState::GAME_OVER) {
                RenderGameOver(renderer, font, SCORE_TIMER);
            }
        }

        SDL_RenderPresent(renderer);
    }

    void SpawnSpears(std::vector<Spear>& spears, const Settings& settings) {
        Spear newSpear;
        int side = rand() % 4;
        if (side == 0) { newSpear.originDirection = Direction::DOWN; newSpear.rect = {rand() % SCREEN_WIDTH, SCREEN_HEIGHT, 5, 15}; }
        else if (side == 1) { newSpear.originDirection = Direction::UP; newSpear.rect = {rand() % SCREEN_WIDTH, -15, 5, 15}; }
        else if (side == 2) { newSpear.originDirection = Direction::RIGHT; newSpear.rect = {SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 15, 5}; }
        else { newSpear.originDirection = Direction::LEFT; newSpear.rect = { -15, rand() % SCREEN_HEIGHT, 15, 5}; }
        spears.push_back(newSpear);
    }

    void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const Settings& settings, GameState& gameState, int& frameCount, float moveX, float moveY) {
        player.x += moveX;
        player.y += moveY;
        player.rect.x = static_cast<int>(player.x - player.rect.w / 2);
        player.rect.y = static_cast<int>(player.y - player.rect.h / 2);

        if (player.rect.x < 0) player.rect.x = 0;
        if (player.rect.x + player.rect.w > SCREEN_WIDTH) player.rect.x = SCREEN_WIDTH - player.rect.w;
        if (player.rect.y < 0) player.rect.y = 0;
        if (player.rect.y + player.rect.h > SCREEN_HEIGHT) player.rect.y = SCREEN_HEIGHT - player.rect.h;
        player.x = player.rect.x + player.rect.w / 2.0f;
        player.y = player.rect.y + player.rect.h / 2.0f;

        frameCount++;
        if (frameCount >= settings.spawnRate) {
            SpawnSpears(spears, settings);
            frameCount = 0;
        }

        // update spear positions
        for (auto& spear : spears) {
            switch (spear.originDirection) {
                case Direction::UP: spear.rect.y += settings.spearSpeed; break;
                case Direction::DOWN: spear.rect.y -= settings.spearSpeed; break;
                case Direction::LEFT: spear.rect.x += settings.spearSpeed; break;
                case Direction::RIGHT: spear.rect.x -= settings.spearSpeed; break;
                case Direction::NONE: break;
            }
        }

        // remove spears that are out of bounds
        for (int i = spears.size() - 1; i >= 0; --i) {
            if (spears[i].rect.x < -100 || spears[i].rect.x > SCREEN_WIDTH + 100 ||
                spears[i].rect.y < -100 || spears[i].rect.y > SCREEN_HEIGHT + 100) {
                spears.erase(spears.begin() + i);
            }
        }

        // check for collisions
        for (auto& spear : spears) {
            if (SDL_HasIntersection(&player.rect, &spear.rect)) {
                gameOver = true;
                gameState = GameState::GAME_OVER;
                break;
            }
        }
    }
}
