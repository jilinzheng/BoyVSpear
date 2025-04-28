#include "spear_dodger.h"
#include <ctime>   // For time()


const int SCREEN_WIDTH = 277;
const int SCREEN_HEIGHT = 277;
int RETURN_TO_MENU = 0; // Flag to return to menu
const int BLOCK_ZONE_SIZE = PLAYER_SIZE + 10; // Keep block zone relative
const int SPEAR_BASE_WIDTH = 10;
const int SPEAR_LENGTH = 40;
const char* FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; // CHANGE THIS to a valid TTF font path!

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

    std::vector<Dodge_Spear> spears;

    // --- Game Loop ---
    while (running) {
        startGame = false;

        HandleInput(running, player, gameState, menuSelectedOption, difficulty, startGame);

        switch (gameState) {
            case GameState::MENU:
                if (startGame) {
                    currentSettings = GetSettingsForDifficulty(difficulty);
                    if (RETURN_TO_MENU==1)
                    {
                        return -1;
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
                    if (frameCount >= currentSettings.spawnRate) {
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

    CloseSDL(window, renderer, font);
    return 0;
}

// --- Function Definitions ---

GameSettings GetSettingsForDifficulty(Difficulty difficulty) {
    GameSettings settings;
    switch (difficulty) {
        case Difficulty::EASY:   settings.spearSpeed = 2; settings.spawnRate = 70; break;
        case Difficulty::MEDIUM: settings.spearSpeed = 3; settings.spawnRate = 50; break;
        case Difficulty::HARD:   settings.spearSpeed = 5; settings.spawnRate = 35; break;
    }
    return settings;
}

void CloseSDL(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font) {
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void ResetGame(Player& player, std::vector<Dodge_Spear>& spears, GameState& gameState, const GameSettings& settings) {
    player.x = static_cast<float>(SCREEN_WIDTH / 2);
    player.y = static_cast<float>(SCREEN_HEIGHT / 2);
    player.rect.x = static_cast<int>(player.x - player.rect.w / 2.0f);
    player.rect.y = static_cast<int>(player.y - player.rect.h / 2.0f);
    player.facing = Direction::UP;
    spears.clear();
    gameState = GameState::PLAYING;
}

void HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) { running = false; }

        if (gameState == GameState::MENU) {
            if (e.type == SDL_KEYDOWN) {
                 switch (e.key.keysym.sym) {
                    case SDLK_UP:    case SDLK_w: selectedOption = (selectedOption - 1 + 4) % 4; break;
                    case SDLK_DOWN:  case SDLK_s: selectedOption = (selectedOption + 1) % 4; break;
                    case SDLK_RETURN: case SDLK_SPACE:
                        if (selectedOption == 0) difficulty = Difficulty::EASY;
                        else if (selectedOption == 1) difficulty = Difficulty::MEDIUM;
                        else if (selectedOption==2) difficulty = Difficulty::HARD;
                        else RETURN_TO_MENU = true;
                        startGame = true;
                        break;
                    case SDLK_ESCAPE: running = false; break;
                }
            }
        } else if (gameState == GameState::PLAYING) {
            if (e.type == SDL_KEYDOWN) {
                 switch (e.key.keysym.sym) {
                    case SDLK_UP:    case SDLK_w: player.facing = Direction::UP; break;
                    case SDLK_DOWN:  case SDLK_s: player.facing = Direction::DOWN; break;
                    case SDLK_LEFT:  case SDLK_a: player.facing = Direction::LEFT; break;
                    case SDLK_RIGHT: case SDLK_d: player.facing = Direction::RIGHT; break;
                    case SDLK_ESCAPE: gameState = GameState::MENU; break;
                 }
            }
        } else if (gameState == GameState::GAME_OVER) {
             if (e.type == SDL_KEYDOWN) {
                 gameState = GameState::MENU;
                 selectedOption = 0;
             }
        }
    }
}

void UpdateGame(Player& player, std::vector<Dodge_Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const GameSettings& settings) {
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

bool CheckSpearInBlockZone(const Dodge_Spear& spear, const SDL_Rect& blockZone) {
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

void SpawnSpear(std::vector<Dodge_Spear>& spears, const GameSettings& settings) {
    Dodge_Spear newSpear;
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

void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Dodge_Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (gameState == GameState::MENU) {
        RenderMenu(renderer, font, selectedOption);
    } else if (gameState == GameState::PLAYING || gameState == GameState::GAME_OVER) {
        // --- Render Gameplay Elements ---
        RenderPlayerCharacter(renderer, player, gameOverFlag, 1); // Draws character AND shield indicator

        // Draw Spears
        SDL_SetRenderDrawColor(renderer, 0, 180, 255, 255);
        for (const auto& spear : spears) {
            RenderSpear(renderer, spear);
        }

        // --- Render Game Over Text ---
        if (gameState == GameState::GAME_OVER) {
            RenderGameOver(renderer, font);
        }
    }

    SDL_RenderPresent(renderer);
}


void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { SDL_FreeSurface(surface); return; }
    SDL_Rect destRect = { x - surface->w / 2, y - surface->h / 2, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    RenderText(renderer, font, "Select Difficulty", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4 - 30, white);
    RenderText(renderer, font, "Easy",   SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30, (selectedOption == 0) ? yellow : white);
    RenderText(renderer, font, "Medium", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 10, (selectedOption == 1) ? yellow : white);
    RenderText(renderer, font, "Hard",   SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 50, (selectedOption == 2) ? yellow : white);
    RenderText(renderer, font, "Back",   SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 90, (selectedOption == 3) ? yellow : white);
}

void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color white = {255, 255, 255, 255};
    RenderText(renderer, font, "GAME OVER", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, red);
    RenderText(renderer, font, "Press any key to return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20, white);
}


void RenderSpear(SDL_Renderer* renderer, const Dodge_Spear& spear) {
    int x = spear.rect.x, y = spear.rect.y, w = spear.rect.w, h = spear.rect.h;
    SDL_Vertex vertex[3];
    vertex[0].color = vertex[1].color = vertex[2].color = {0, 180, 255, 255}; // Spear color

    switch (spear.originDirection) {
        case Direction::UP:    vertex[0].position = {(float)x, (float)y}; vertex[1].position = {(float)(x + w), (float)y}; vertex[2].position = {(float)(x + w / 2), (float)(y + h)}; break;
        case Direction::DOWN:  vertex[0].position = {(float)x, (float)(y + h)}; vertex[1].position = {(float)(x + w), (float)(y + h)}; vertex[2].position = {(float)(x + w / 2), (float)y}; break;
        case Direction::LEFT:  vertex[0].position = {(float)x, (float)y}; vertex[1].position = {(float)x, (float)(y + h)}; vertex[2].position = {(float)(x + w), (float)(y + h / 2)}; break;
        case Direction::RIGHT: vertex[0].position = {(float)(x + w), (float)y}; vertex[1].position = {(float)(x + w), (float)(y + h)}; vertex[2].position = {(float)x, (float)(y + h / 2)}; break;
        case Direction::NONE: return;
    }
    SDL_RenderGeometry(renderer, nullptr, vertex, 3, nullptr, 0);
}
