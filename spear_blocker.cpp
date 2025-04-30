#include "spear_blocker.h"


int SPEAR_COUNTER = 0; // Counter for spears
int RETURN_TO_MENU = 0; // Flag to return to menu
const int BLOCK_ZONE_SIZE = PLAYER_SIZE + 20; // Keep block zone relative


// --- Main Function ---
int SpearDodgerMain(SDL_Window* window, SDL_Renderer* renderer) {
    TTF_Font* font = nullptr;

    font = TTF_OpenFont(FONT_PATH, 28);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        printf("Ensure the path '%s' is correct.\n", FONT_PATH);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return false;
    }

    srand(time(0));

    // Game variables
    bool running = true;
    GameState gameState = GameState::MENU;
    Difficulty difficulty = Difficulty::MEDIUM;
    GameSettings currentSettings = GetSettingsForDifficulty(difficulty);
    bool gameOverFlag = false;
    int frameCount = 0;
    int menuSelectedOption = 0;
    bool startGame = false;

    // Initialize Player
    Player player;
    player.x = static_cast<float>(SCREEN_WIDTH / 2);
    player.y = static_cast<float>(SCREEN_HEIGHT / 2);
    player.rect.w = PLAYER_SIZE;
    player.rect.h = PLAYER_SIZE;
    player.rect.x = static_cast<int>(player.x - player.rect.w / 2.0f);
    player.rect.y = static_cast<int>(player.y - player.rect.h / 2.0f);
    player.facing = Direction::UP;

    // Define the central blocking zone
    SDL_Rect blockZone;
    blockZone.w = BLOCK_ZONE_SIZE;
    blockZone.h = BLOCK_ZONE_SIZE;
    blockZone.x = static_cast<int>(player.x - BLOCK_ZONE_SIZE / 2.0f);
    blockZone.y = static_cast<int>(player.y - BLOCK_ZONE_SIZE / 2.0f);

    std::vector<Spear> spears;


    // --- Game Loop ---
    while (running) {

        startGame = false;

        if (HandleInput(running, player, gameState, menuSelectedOption, difficulty, startGame) == -1)
        {
            return -1;
        }

        if (!running) break;

        switch (gameState) {
            case GameState::MENU:
                if (startGame) {
                    currentSettings = GetSettingsForDifficulty(difficulty);
                    if (RETURN_TO_MENU==1)
                    {
                        startGame = false;
                        RETURN_TO_MENU = 0;
                        return 0;
                    }
                    ResetGame(player, spears, gameState, currentSettings);
                    gameOverFlag = false;
                    frameCount = 0;
                    blockZone.x = static_cast<int>(player.x - BLOCK_ZONE_SIZE / 2.0f);
                    blockZone.y = static_cast<int>(player.y - BLOCK_ZONE_SIZE / 2.0f);
                }
                break;

            case GameState::PLAYING:
                if (!gameOverFlag) {
                    UpdateGame(player, spears, gameOverFlag, blockZone, currentSettings);

                    frameCount++;
                    if (frameCount >= currentSettings.spawnRate/ currentSettings.spearMult) {
                        SpawnSpear(spears, currentSettings);
                        frameCount = 0;
                    }

                    if (gameOverFlag) {
                        gameState = GameState::GAME_OVER;
                        printf("Game Over!\n");
                    }
                }
                break;

            case GameState::GAME_OVER:
                // Input handling checks for return to menu
                break;
        }

        RenderGame(renderer, font, player, spears, gameState, menuSelectedOption, gameOverFlag);
        SDL_Delay(16);
    }

    return 0;
}

// --- Function Definitions ---

GameSettings GetSettingsForDifficulty(Difficulty difficulty) {
    GameSettings settings;
    switch (difficulty) {
        case Difficulty::EASY:   settings.spearSpeed = 2; settings.spawnRate = 70; settings.spearMult = 1; break;
        case Difficulty::MEDIUM: settings.spearSpeed = 3; settings.spawnRate = 50; settings.spearMult = 2; break;
        case Difficulty::HARD:   settings.spearSpeed = 4; settings.spawnRate = 40; settings.spearMult = 3; break;
    }
    return settings;
}

void ResetGame(Player& player, std::vector<Spear>& spears, GameState& gameState, const GameSettings& settings) {
    player.x = static_cast<float>(SCREEN_WIDTH / 2);
    player.y = static_cast<float>(SCREEN_HEIGHT / 2);
    player.rect.x = static_cast<int>(player.x - player.rect.w / 2.0f);
    player.rect.y = static_cast<int>(player.y - player.rect.h / 2.0f);
    player.facing = Direction::UP;
    spears.clear();
    gameState = GameState::PLAYING;
}

int HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame){
    if (gameState == GameState::MENU) {
        if (joy_action && joy.y == UP) selectedOption = (selectedOption-1+4)%4;
        if (joy_action && joy.y == DOWN) selectedOption = (selectedOption+1)%4;
        if (joy_action && joy.btn == PRESSED) {
            if (selectedOption == 0) difficulty = Difficulty::EASY;
            else if (selectedOption == 1) difficulty = Difficulty::MEDIUM;
            else if (selectedOption==2) difficulty = Difficulty::HARD;
            else RETURN_TO_MENU = true;
            startGame = true;
        }

    } else if (gameState == GameState::PLAYING) {
        if (joy_action && joy.y == UP) player.facing = Direction::UP;
        if (joy_action && joy.y == DOWN) player.facing = Direction::DOWN;
        if (joy_action && joy.x == LEFT) player.facing = Direction::LEFT;
        if (joy_action && joy.x == RIGHT) player.facing = Direction::RIGHT;

    } else if (gameState == GameState::GAME_OVER) {
        if (joy_action && (joy.x!=NEUTRAL||joy.y!=NEUTRAL||joy.btn==PRESSED)) {
            gameState = GameState::MENU;
            selectedOption = 0;
        }
    }
    return 0;
}

void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const GameSettings& settings) {
    for (int i = spears.size() - 1; i >= 0; --i) {
        // Move spear
        switch (spears[i].originDirection) {
            case Direction::UP:    spears[i].y += spears[i].speed; break;
            case Direction::DOWN:  spears[i].y -= spears[i].speed; break;
            case Direction::LEFT:  spears[i].x += spears[i].speed; break;
            case Direction::RIGHT: spears[i].x -= spears[i].speed; break;
            case Direction::NONE:  break;
        }
        spears[i].rect.x = static_cast<int>(spears[i].x);
        spears[i].rect.y = static_cast<int>(spears[i].y);

        // Check collision/block
        if (CheckSpearInBlockZone(spears[i], blockZone)) {
            if (player.facing == spears[i].originDirection) {
                spears.erase(spears.begin() + i); // Blocked
                SPEAR_COUNTER++;
            } else {
                gameOver = true; // Hit
                return;
            }
        }
        // Remove off-screen spears
        else if (spears[i].y < -SPEAR_LENGTH * 2 || spears[i].y > SCREEN_HEIGHT + SPEAR_LENGTH ||
                   spears[i].x < -SPEAR_LENGTH * 2 || spears[i].x > SCREEN_WIDTH + SPEAR_LENGTH) {
             spears.erase(spears.begin() + i);
        }
    }
}

bool CheckSpearInBlockZone(const Spear& spear, const SDL_Rect& blockZone) {
    int tipX = spear.rect.x, tipY = spear.rect.y;
    switch (spear.originDirection) {
        case Direction::UP:    tipX = spear.rect.x + spear.rect.w / 2; tipY = spear.rect.y + spear.rect.h; break;
        case Direction::DOWN:  tipX = spear.rect.x + spear.rect.w / 2; tipY = spear.rect.y; break;
        case Direction::LEFT:  tipX = spear.rect.x + spear.rect.w; tipY = spear.rect.y + spear.rect.h / 2; break;
        case Direction::RIGHT: tipX = spear.rect.x; tipY = spear.rect.y + spear.rect.h / 2; break;
        case Direction::NONE: return false;
    }
    return (tipX >= blockZone.x && tipX < blockZone.x + blockZone.w &&
            tipY >= blockZone.y && tipY < blockZone.y + blockZone.h);
}

void SpawnSpear(std::vector<Spear>& spears, const GameSettings& settings) {
    Spear newSpear;
    newSpear.speed = settings.spearSpeed;
    int side = rand() % 4;
    int width, height;

    if (side == 0 || side == 1) { width = SPEAR_BASE_WIDTH; height = SPEAR_LENGTH; }
    else { width = SPEAR_LENGTH; height = SPEAR_BASE_WIDTH; }
    newSpear.rect.w = width; newSpear.rect.h = height;

    float spawnX = 0, spawnY = 0;
    float targetX = static_cast<float>(SCREEN_WIDTH / 2);
    float targetY = static_cast<float>(SCREEN_HEIGHT / 2);

    switch (side) {
        case 0: newSpear.originDirection = Direction::UP;    spawnX = targetX - width / 2.0f; spawnY = static_cast<float>(-height); break;
        case 1: newSpear.originDirection = Direction::DOWN;  spawnX = targetX - width / 2.0f; spawnY = static_cast<float>(SCREEN_HEIGHT); break;
        case 2: newSpear.originDirection = Direction::LEFT;  spawnX = static_cast<float>(-width); spawnY = targetY - height / 2.0f; break;
        case 3: newSpear.originDirection = Direction::RIGHT; spawnX = static_cast<float>(SCREEN_WIDTH); spawnY = targetY - height / 2.0f; break;
    }
    newSpear.x = spawnX; newSpear.y = spawnY;
    newSpear.rect.x = static_cast<int>(newSpear.x); newSpear.rect.y = static_cast<int>(newSpear.y);
    spears.push_back(newSpear);
}

void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (gameState == GameState::MENU) {
        if (font) RenderMenu(renderer, font, selectedOption);
    } else if (gameState == GameState::PLAYING || gameState == GameState::GAME_OVER) {
        if (renderer) {
            RenderPlayerCharacter(renderer, player, gameOverFlag, 1);
            SDL_SetRenderDrawColor(renderer, 0, 180, 255, 255);
            for (const auto& spear : spears) {
                RenderSpear(renderer, spear);
            }
            RenderScore(renderer, font, SPEAR_COUNTER);  // << Only one simple call now
        }

        if (gameState == GameState::GAME_OVER) {
            if (font) RenderGameOver(renderer, font, SPEAR_COUNTER);
        }
    }

    SDL_RenderPresent(renderer);
}
