#include "spear_runner.h"
#include "assets.h"
#include <cstdlib>
#include <ctime>

int SCORE_TIMER = 0;
Uint32 lastIncrementTime = SDL_GetTicks();  // current time in milliseconds


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
    TTF_Font* font = nullptr;
    font = TTF_OpenFont(FONT_PATH, 28);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        printf("Ensure the path '%s' is correct.\n", FONT_PATH);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return false;
    }

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
        if (gameState == SpearRunnerGameState::MENU) {
            std::lock_guard<std::mutex> lock(joy_mutex);
            if (joy_action) {
                joy_action = false;
                if (joy.y == UP) selectedOption = (selectedOption - 1 + 4) % 4;
                else if (joy.y == DOWN) selectedOption = (selectedOption + 1) % 4;
                else if (joy.btn == PRESSED) {
                    if (selectedOption == 3) {
                        return 0; // Back selected
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
            }
        }

        else if (gameState == SpearRunnerGameState::GAME_OVER) {
            std::lock_guard<std::mutex> lock(joy_mutex);
            if (joy_action) {
                joy_action = false;
                if (joy.btn==PRESSED)
                    gameState = SpearRunnerGameState::MENU;
            }
        }

        // Gameplay logic
        if (gameState == SpearRunnerGameState::PLAYING && !gameOver) {
            // std::lock_guard<std::mutex> lock(joy_mutex);
            // update score
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime > lastIncrementTime + 1000) { // 1000 ms = 1 second
                SCORE_TIMER++;
                lastIncrementTime = currentTime;
            }

            // const Uint8* keys = SDL_GetKeyboardState(NULL);
            float moveX = 0, moveY = 0;
            // if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) moveY = -PLAYER_SPEED;
            // if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) moveY = PLAYER_SPEED;
            // if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) moveX = -PLAYER_SPEED;
            // if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) moveX = PLAYER_SPEED;
            if (joy.y == UP) moveY = -PLAYER_SPEED;
            if (joy.y == DOWN) moveY = PLAYER_SPEED;
            if (joy.x == LEFT) moveX = -PLAYER_SPEED;
            if (joy.x == RIGHT) moveX = PLAYER_SPEED;

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
                if (side == 0) { newSpear.originDirection = Direction::DOWN; newSpear.rect = {rand() % SCREEN_WIDTH, SCREEN_HEIGHT, 5, 15}; }
                else if (side == 1) { newSpear.originDirection = Direction::UP; newSpear.rect = {rand() % SCREEN_WIDTH, -15, 5, 15}; }
                else if (side == 2) { newSpear.originDirection = Direction::RIGHT; newSpear.rect = {SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 15, 5}; }
                else { newSpear.originDirection = Direction::LEFT; newSpear.rect = { -15, rand() % SCREEN_HEIGHT, 15, 5}; }
                spears.push_back(newSpear);
                frameCount = 0;
            }

            for (auto& spear : spears) {
                switch (spear.originDirection) {
                    case Direction::UP: spear.rect.y += settings.spearSpeed; break;
                    case Direction::DOWN: spear.rect.y -= settings.spearSpeed; break;
                    case Direction::LEFT: spear.rect.x += settings.spearSpeed; break;
                    case Direction::RIGHT: spear.rect.x -= settings.spearSpeed; break;
                    case Direction::NONE: break;
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
            RenderMenu(renderer, font, selectedOption);
            }
         else {
            RenderPlayerCharacter(renderer, player, gameOver, 0);
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            RenderScore(renderer, font, SCORE_TIMER); // Render score
            for (auto& spear : spears) {
                RenderSpear(renderer, spear); // Draw spears
            }
            if (gameState == SpearRunnerGameState::GAME_OVER) {
                RenderGameOver(renderer, font, SCORE_TIMER);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}

