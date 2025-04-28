#include "spear_runner.h"
#include "Character.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <cstdlib>
#include <ctime>

static const int SCREEN_WIDTH = 277;
static const int SCREEN_HEIGHT = 277;

RunnerSettings GetSettingsForDifficulty(SpearRunnerDifficulty difficulty) {
    RunnerSettings settings;
    switch (difficulty) {
        case SpearRunnerDifficulty::EASY: settings.spearSpeed = 2; settings.spawnRate = 60; break;
        case SpearRunnerDifficulty::MEDIUM: settings.spearSpeed = 4; settings.spawnRate = 40; break;
        case SpearRunnerDifficulty::HARD: settings.spearSpeed = 6; settings.spawnRate = 20; break;
        default: settings.spearSpeed = 4; settings.spawnRate = 40; break;
    }
    return settings;
}

int SpearRunnerMain(SDL_Window* window, SDL_Renderer* renderer) {
    srand(time(0));

    SpearRunnerGameState gameState = SpearRunnerGameState::MENU;
    int selectedOption = 1; // 0=Easy, 1=Medium, 2=Hard, 3=Back
    bool gameOver = false;
    int frameCount = 0;
    RunnerSettings settings = GetSettingsForDifficulty(SpearRunnerDifficulty::MEDIUM);

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
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return -1;
            if (e.type == SDL_KEYDOWN) {
                if (gameState == SpearRunnerGameState::MENU) {
                    if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP)
                        selectedOption = (selectedOption - 1 + 4) % 4;
                    else if (e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN)
                        selectedOption = (selectedOption + 1) % 4;
                    else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
                        if (selectedOption == 3) {
                            return -1; // Back selected
                        } else {
                            settings = GetSettingsForDifficulty(static_cast<SpearRunnerDifficulty>(selectedOption));
                            gameOver = false;
                            frameCount = 0;
                            spears.clear();
                            player.x = SCREEN_WIDTH / 2.0f;
                            player.y = SCREEN_HEIGHT / 2.0f;
                            player.rect.x = static_cast<int>(player.x - player.rect.w / 2);
                            player.rect.y = static_cast<int>(player.y - player.rect.h / 2);
                            gameState = SpearRunnerGameState::PLAYING;
                        }
                    }
                } else if (gameState == SpearRunnerGameState::GAME_OVER) {
                    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
                        gameState = SpearRunnerGameState::MENU;
                    }
                }
            }
        }

        // Gameplay logic
        if (gameState == SpearRunnerGameState::PLAYING && !gameOver) {
            const Uint8* keys = SDL_GetKeyboardState(NULL);
            float moveX = 0, moveY = 0;
            if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) moveY = -PLAYER_SPEED;
            if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) moveY = PLAYER_SPEED;
            if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) moveX = -PLAYER_SPEED;
            if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) moveX = PLAYER_SPEED;

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
                Spear newSpear;
                int side = rand() % 4;
                if (side == 0) { newSpear.direction = SpearDirection::DOWN; newSpear.rect = {rand() % SCREEN_WIDTH, -15, 5, 15}; }
                else if (side == 1) { newSpear.direction = SpearDirection::UP; newSpear.rect = {rand() % SCREEN_WIDTH, SCREEN_HEIGHT, 5, 15}; }
                else if (side == 2) { newSpear.direction = SpearDirection::RIGHT; newSpear.rect = {-15, rand() % SCREEN_HEIGHT, 15, 5}; }
                else { newSpear.direction = SpearDirection::LEFT; newSpear.rect = {SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 15, 5}; }
                spears.push_back(newSpear);
                frameCount = 0;
            }

            for (auto& spear : spears) {
                switch (spear.direction) {
                    case SpearDirection::UP: spear.rect.y -= settings.spearSpeed; break;
                    case SpearDirection::DOWN: spear.rect.y += settings.spearSpeed; break;
                    case SpearDirection::LEFT: spear.rect.x -= settings.spearSpeed; break;
                    case SpearDirection::RIGHT: spear.rect.x += settings.spearSpeed; break;
                }
            }

            for (int i = spears.size() - 1; i >= 0; --i) {
                if (spears[i].rect.x < -100 || spears[i].rect.x > SCREEN_WIDTH + 100 ||
                    spears[i].rect.y < -100 || spears[i].rect.y > SCREEN_HEIGHT + 100) {
                    spears.erase(spears.begin() + i);
                }
            }

            for (auto& spear : spears) {
                if (SDL_HasIntersection(&player.rect, &spear.rect)) {
                    gameOver = true;
                    gameState = SpearRunnerGameState::GAME_OVER;
                    break;
                }
            }
        }

        // --- Render ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState == SpearRunnerGameState::MENU) {
            SDL_Color white = {255, 255, 255, 255};
            SDL_Color yellow = {255, 255, 0, 255};
            TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
            if (font) {
                SDL_Surface* surface = TTF_RenderText_Blended(font, "Select Difficulty", white);
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect rect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 4 - 30, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_FreeSurface(surface);
                SDL_DestroyTexture(texture);

                const char* options[4] = {"Easy", "Medium", "Hard", "Back"};
                for (int i = 0; i < 4; ++i) {
                    surface = TTF_RenderText_Blended(font, options[i], i == selectedOption ? yellow : white);
                    texture = SDL_CreateTextureFromSurface(renderer, surface);
                    rect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 + i * 30 - 60, surface->w, surface->h};
                    SDL_RenderCopy(renderer, texture, NULL, &rect);
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                }
                TTF_CloseFont(font);
            }
        } else {
            RenderPlayerCharacter(renderer, player, gameOver, 0);
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            for (auto& spear : spears) {
                SDL_RenderFillRect(renderer, &spear.rect);
            }
            if (gameState == SpearRunnerGameState::GAME_OVER) {
                TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
                if (font) {
                    SDL_Color red = {255, 0, 0, 255};
                    SDL_Surface* surface = TTF_RenderText_Blended(font, "GAME OVER", red);
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_Rect rect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 - 20, surface->w, surface->h};
                    SDL_RenderCopy(renderer, texture, NULL, &rect);
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                    TTF_CloseFont(font);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}
    