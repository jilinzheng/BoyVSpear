#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Include SDL_ttf for text rendering
#include <vector>
#include <string>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <cmath>   // For abs()
#include "pigpio.h"


// --- Configuration ---
const int SCREEN_WIDTH = 272;
const int SCREEN_HEIGHT = 272;
const int PLAYER_SIZE = 5;
const int BLOCK_ZONE_SIZE = PLAYER_SIZE + 10;
const int SPEAR_BASE_WIDTH = 5; // Width of the spear's base
const int SPEAR_LENGTH = 20;    // Length from base to tip
const int INDICATOR_SIZE = 5;
const char* FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; // CHANGE THIS to a valid TTF font path on your system!
                                                                       // Common paths: Windows: "C:/Windows/Fonts/arial.ttf"
                                                                       // macOS: "/Library/Fonts/Arial.ttf"
                                                                       // Linux: Check /usr/share/fonts/ or similar
const int GPIO_BTN_LEFT = 16;
const int GPIO_BTN_UP = 20;
const int GPIO_BTN_RIGHT = 21;
const int GPIO_BTN_DOWN = 19;
const int GPIO_BTN_ACTION = 26;
static bool prev_gpio_up = true;    // Assume buttons start unpressed (HIGH with pull-up)
static bool prev_gpio_down = true;
static bool prev_gpio_left = true;
static bool prev_gpio_right = true;
static bool prev_gpio_action = true;


// --- Enums ---
enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER
};

enum class Direction {
    NONE, UP, DOWN, LEFT, RIGHT
};

enum class Difficulty {
    EASY, MEDIUM, HARD
};

// --- Structs ---
struct Player {
    SDL_Rect rect;
    Direction facing = Direction::UP;
};

struct Spear {
    SDL_Rect rect; // Used for position and rough collision bounds
    Direction originDirection;
    float x, y; // Use float for smoother movement calculation
    int speed;
};

struct GameSettings {
    int spearSpeed;
    int spawnRate;
};


// --- Function Prototypes ---
bool InitializeSDL(SDL_Window*& window, SDL_Renderer*& renderer, TTF_Font*& font);
void CloseSDL(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font);
void HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame);
void ResetGame(Player& player, std::vector<Spear>& spears, GameState& gameState, const GameSettings& settings);
void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const GameSettings& settings);
void SpawnSpear(std::vector<Spear>& spears, const GameSettings& settings);
void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOver);
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption);
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font);
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
void RenderSpear(SDL_Renderer* renderer, const Spear& spear); // New function for spear rendering
bool CheckSpearInBlockZone(const Spear& spear, const SDL_Rect& blockZone);
GameSettings GetSettingsForDifficulty(Difficulty difficulty);


// --- Main Function ---
int main(int argc, char* args[]) {
    // set up GPIO
    int status = gpioInitialise();
    if (status < 0) {
        fprintf(stderr, "pigpio initialization failed.\n");
        return 1;
    }
    gpioSetMode(GPIO_BTN_LEFT, PI_INPUT);
    gpioSetMode(GPIO_BTN_UP, PI_INPUT);
    gpioSetMode(GPIO_BTN_RIGHT, PI_INPUT);
    gpioSetMode(GPIO_BTN_DOWN, PI_INPUT);
    gpioSetMode(GPIO_BTN_ACTION, PI_INPUT);

    gpioSetPullUpDown(GPIO_BTN_LEFT, PI_PUD_UP);
    gpioSetPullUpDown(GPIO_BTN_UP, PI_PUD_UP);
    gpioSetPullUpDown(GPIO_BTN_RIGHT, PI_PUD_UP);
    gpioSetPullUpDown(GPIO_BTN_DOWN, PI_PUD_UP);
    gpioSetPullUpDown(GPIO_BTN_ACTION, PI_PUD_UP);

    // set up window and renderer
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    if (!InitializeSDL(window, renderer, font)) {
        printf("Failed to initialize SDL or TTF!\n");
        return 1;
    }

    srand(time(0));

    // Game variables
    bool running = true;
    GameState gameState = GameState::MENU;
    Difficulty difficulty = Difficulty::MEDIUM; // Default difficulty
    GameSettings currentSettings = GetSettingsForDifficulty(difficulty);
    bool gameOverFlag = false; // Separate flag for game over condition within PLAYING state
    int frameCount = 0;
    int menuSelectedOption = 0; // 0: Easy, 1: Medium, 2: Hard
    bool startGame = false; // Flag to trigger game start from menu input

    // Initialize Player
    Player player;
    player.rect.w = PLAYER_SIZE;
    player.rect.h = PLAYER_SIZE;
    player.rect.x = (SCREEN_WIDTH - PLAYER_SIZE) / 2;
    player.rect.y = (SCREEN_HEIGHT - PLAYER_SIZE) / 2;
    player.facing = Direction::UP;

    // Define the central blocking zone
    SDL_Rect blockZone;
    blockZone.w = BLOCK_ZONE_SIZE;
    blockZone.h = BLOCK_ZONE_SIZE;
    blockZone.x = (SCREEN_WIDTH - BLOCK_ZONE_SIZE) / 2;
    blockZone.y = (SCREEN_HEIGHT - BLOCK_ZONE_SIZE) / 2;

    std::vector<Spear> spears;

    // --- Game Loop ---
    while (running) {
        // not condition due to PULL-UP resistor
        // if (!gpioRead(GPIO_BTN_LEFT)) printf("LEFT PRESSED!\n");
        // if (!gpioRead(GPIO_BTN_UP)) printf("UPPRESSED!\n");
        // if (!gpioRead(GPIO_BTN_RIGHT)) printf("RIGHT PRESSED!\n");
        // if (!gpioRead(GPIO_BTN_DOWN)) printf("DOWN PRESSED!\n");
        // if (!gpioRead(GPIO_BTN_ACTION)) printf("ACTION PRESSED!\n");

        startGame = false; // Reset start trigger each frame

        // --- Input Handling (Handles state transitions) ---
        HandleInput(running, player, gameState, menuSelectedOption, difficulty, startGame);
        // // GPIO instead of SDL handling
        // if (gameState == GameState::MENU) {
        //     if (!gpioRead(GPIO_BTN_UP)) menuSelectedOption = (menuSelectedOption - 1 + 3) % 3; // Cycle up (0, 2, 1, 0...)
        //     if (!gpioRead(GPIO_BTN_DOWN)) menuSelectedOption = (menuSelectedOption + 1) % 3; // Cycle down (0, 1, 2, 0...)
        //     if (!gpioRead(GPIO_BTN_ACTION)) {
        //         if (menuSelectedOption == 0) difficulty = Difficulty::EASY;
        //         else if (menuSelectedOption == 1) difficulty = Difficulty::MEDIUM;
        //         else difficulty = Difficulty::HARD;
        //         startGame = true; // Signal to start the game in the main loop
        //     }
        // } else if (gameState == GameState::PLAYING) {
        //     if (!gpioRead(GPIO_BTN_UP)) player.facing = Direction::UP;
        //     if (!gpioRead(GPIO_BTN_DOWN)) player.facing = Direction::DOWN;
        //     if (!gpioRead(GPIO_BTN_LEFT)) player.facing = Direction::LEFT;
        //     if (!gpioRead(GPIO_BTN_RIGHT)) player.facing = Direction::RIGHT;
        // }

        // --- Add Debouncing Logic ---
        bool current_gpio_up = gpioRead(GPIO_BTN_UP);
        bool current_gpio_down = gpioRead(GPIO_BTN_DOWN);
        bool current_gpio_left = gpioRead(GPIO_BTN_LEFT);
        bool current_gpio_right = gpioRead(GPIO_BTN_RIGHT);
        bool current_gpio_action = gpioRead(GPIO_BTN_ACTION);

        // Check for a "rising edge" (transition from unpressed to pressed) for each button
        bool gpio_up_pressed = !current_gpio_up && prev_gpio_up;
        bool gpio_down_pressed = !current_gpio_down && prev_gpio_down;
        bool gpio_left_pressed = !current_gpio_left && prev_gpio_left;
        bool gpio_right_pressed = !current_gpio_right && prev_gpio_right;
        bool gpio_action_pressed = !current_gpio_action && prev_gpio_action;

        // --- Update Game Logic based on Debounced Presses ---

        // GPIO instead of SDL handling (Modified to use debounced presses)
        if (gameState == GameState::MENU) {
            // Menu navigation only on a distinct button press
            if (gpio_up_pressed) {
                menuSelectedOption = (menuSelectedOption - 1 + 3) % 3; // Cycle up
            }
            if (gpio_down_pressed) {
                menuSelectedOption = (menuSelectedOption + 1) % 3; // Cycle down
            }
            // Menu selection confirmation on action button press
            if (gpio_action_pressed) {
                if (menuSelectedOption == 0) difficulty = Difficulty::EASY;
                else if (menuSelectedOption == 1) difficulty = Difficulty::MEDIUM;
                else difficulty = Difficulty::HARD;
                startGame = true; // Signal to start the game in the main loop
            }
        } else if (gameState == GameState::PLAYING) {
            // For continuous movement in PLAYING state, you might want to use the *current* state
            // or a combination. Using the debounced *press* for facing direction change
            // means you only change direction on a new press, not while holding.
            // If you want continuous movement while holding the button, you would use
            // the `current_gpio_...` variables here instead of the `..._pressed` ones.
            // Let's use the current state for movement facing, as that's more typical for this game style.
            // The debounced presses are still useful for menu navigation and actions.

            if (!current_gpio_up) player.facing = Direction::UP; // Button held down (LOW)
            else if (!current_gpio_down) player.facing = Direction::DOWN; // Button held down (LOW)
            else if (!current_gpio_left) player.facing = Direction::LEFT; // Button held down (LOW)
            else if (!current_gpio_right) player.facing = Direction::RIGHT; // Button held down (LOW)
            // If multiple are held, the order of these checks determines priority.
            // If none are held, player.facing remains the last set direction.

            // Example: Use action button to return to menu during gameplay (debounced press)
            if (gpio_action_pressed) {
                gameState = GameState::MENU;
                // Optionally reset game state here or when re-entering PLAYING
            }

        } else if (gameState == GameState::GAME_OVER) {
            // Return to menu on action button press (debounced)
            if (gpio_action_pressed) {
                gameState = GameState::MENU;
                menuSelectedOption = 0; // Reset menu selection
            }
        }

        // --- IMPORTANT: Update Previous State for Next Frame ---
        // This must happen after you've used the current states for this frame's logic.
        prev_gpio_up = current_gpio_up;
        prev_gpio_down = current_gpio_down;
        prev_gpio_left = current_gpio_left;
        prev_gpio_right = current_gpio_right;
        prev_gpio_action = current_gpio_action;

        // --- State Logic ---
        switch (gameState) {
            case GameState::MENU:
                if (startGame) {
                    currentSettings = GetSettingsForDifficulty(difficulty);
                    ResetGame(player, spears, gameState, currentSettings); // Resets and sets state to PLAYING
                    gameOverFlag = false;
                    frameCount = 0;
                }
                break;

            case GameState::PLAYING:
                if (!gameOverFlag) {
                    // --- Update ---
                    UpdateGame(player, spears, gameOverFlag, blockZone, currentSettings);

                    // Spawn spears periodically
                    frameCount++;
                    if (frameCount >= currentSettings.spawnRate) {
                        SpawnSpear(spears, currentSettings);
                        frameCount = 0; // Reset counter
                    }

                    if (gameOverFlag) {
                        gameState = GameState::GAME_OVER; // Transition to game over state
                        printf("Game Over!\n");
                    }
                }
                break;

            case GameState::GAME_OVER:
                // Input handling already checks for key press to return to menu
                break;
        }

        // --- Rendering ---
        RenderGame(renderer, font, player, spears, gameState, menuSelectedOption, gameOverFlag);

        // Delay
        SDL_Delay(16); // Roughly 60 FPS
    }

    // --- Cleanup ---
    CloseSDL(window, renderer, font);
    gpioTerminate();

    return 0;
}

// --- Function Definitions ---

// Get game settings based on difficulty
GameSettings GetSettingsForDifficulty(Difficulty difficulty) {
    GameSettings settings;
    switch (difficulty) {
        case Difficulty::EASY:
            settings.spearSpeed = 2;
            settings.spawnRate = 70;
            break;
        case Difficulty::MEDIUM:
            settings.spearSpeed = 3;
            settings.spawnRate = 50;
            break;
        case Difficulty::HARD:
            settings.spearSpeed = 5;
            settings.spawnRate = 35;
            break;
    }
    return settings;
}


// Initialize SDL, TTF, create window, renderer, and load font
bool InitializeSDL(SDL_Window*& window, SDL_Renderer*& renderer, TTF_Font*& font) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    window = SDL_CreateWindow("Spear Block (Undertale Inspired)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // Load font - ADJUST SIZE AS NEEDED
    font = TTF_OpenFont(FONT_PATH, 28); // Load font with size 28
    if (font == nullptr) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        printf("Ensure the path '%s' is correct and the font file exists.\n", FONT_PATH);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

// Clean up SDL and TTF resources
void CloseSDL(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font) {
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    font = nullptr;
    renderer = nullptr;
    window = nullptr;
    TTF_Quit();
    SDL_Quit();
}

// Reset game state for starting a new game
void ResetGame(Player& player, std::vector<Spear>& spears, GameState& gameState, const GameSettings& settings) {
    player.rect.x = (SCREEN_WIDTH - PLAYER_SIZE) / 2;
    player.rect.y = (SCREEN_HEIGHT - PLAYER_SIZE) / 2;
    player.facing = Direction::UP; // Reset facing direction
    spears.clear(); // Remove all existing spears
    gameState = GameState::PLAYING; // Set state to playing
}

// Handle user input events based on game state
void HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame) {
    SDL_Event e;

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            running = false;
        }

        if (gameState == GameState::MENU) {
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                    case SDLK_w:
                        selectedOption = (selectedOption - 1 + 3) % 3; // Cycle up (0, 2, 1, 0...)
                        break;
                    case SDLK_DOWN:
                    case SDLK_s:
                        selectedOption = (selectedOption + 1) % 3; // Cycle down (0, 1, 2, 0...)
                        break;
                    case SDLK_RETURN: // Enter key
                    case SDLK_SPACE:
                        if (selectedOption == 0) difficulty = Difficulty::EASY;
                        else if (selectedOption == 1) difficulty = Difficulty::MEDIUM;
                        else difficulty = Difficulty::HARD;
                        startGame = true; // Signal to start the game in the main loop
                        break;
                    case SDLK_ESCAPE: // Allow quitting from menu
                        running = false;
                        break;
                }
            }
        } else if (gameState == GameState::PLAYING) {
            if (e.type == SDL_KEYDOWN) {
                 switch (e.key.keysym.sym) {
                    case SDLK_UP:    case SDLK_w: player.facing = Direction::UP; break;
                    case SDLK_DOWN:  case SDLK_s: player.facing = Direction::DOWN; break;
                    case SDLK_LEFT:  case SDLK_a: player.facing = Direction::LEFT; break;
                    case SDLK_RIGHT: case SDLK_d: player.facing = Direction::RIGHT; break;
                    case SDLK_ESCAPE: gameState = GameState::MENU; break; // Go back to menu
                 }
            }
        } else if (gameState == GameState::GAME_OVER) {
             if (e.type == SDL_KEYDOWN) {
                 // Any key press returns to menu
                 gameState = GameState::MENU;
                 selectedOption = 0; // Reset menu selection
             }
        }
    }
}

// Update spear positions and check for blocks or game over
void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver, const SDL_Rect& blockZone, const GameSettings& settings) {
    // Update spear positions and check for blocking/game over
    for (int i = spears.size() - 1; i >= 0; --i) {
        // Move spear based on its origin direction (towards center)
        switch (spears[i].originDirection) {
            case Direction::UP:    spears[i].y += spears[i].speed; break;
            case Direction::DOWN:  spears[i].y -= spears[i].speed; break;
            case Direction::LEFT:  spears[i].x += spears[i].speed; break;
            case Direction::RIGHT: spears[i].x -= spears[i].speed; break;
            case Direction::NONE:  break;
        }
        // Update the rect position based on the float values for collision checking
        spears[i].rect.x = static_cast<int>(spears[i].x);
        spears[i].rect.y = static_cast<int>(spears[i].y);


        // Check if spear has reached the central block zone
        if (CheckSpearInBlockZone(spears[i], blockZone)) {
            if (player.facing == spears[i].originDirection) {
                // Block successful! Remove the spear.
                spears.erase(spears.begin() + i);
            } else {
                // Block failed! Game Over.
                gameOver = true;
                return; // Stop processing further spears this frame
            }
        }
        // Remove spears that go way off screen
        else if (spears[i].y < -SPEAR_LENGTH * 2 || spears[i].y > SCREEN_HEIGHT + SPEAR_LENGTH ||
                 spears[i].x < -SPEAR_LENGTH * 2 || spears[i].x > SCREEN_WIDTH + SPEAR_LENGTH)
        {
             spears.erase(spears.begin() + i);
        }
    }
}

// Check if a spear's *tip* is within the central block zone
bool CheckSpearInBlockZone(const Spear& spear, const SDL_Rect& blockZone) {
    // Calculate spear tip position based on origin
    int tipX = spear.rect.x, tipY = spear.rect.y; // Default to top-left for simplicity, adjust based on direction

    switch (spear.originDirection) {
        case Direction::UP: // Moving DOWN
            tipX = spear.rect.x + spear.rect.w / 2; // Mid-point of base
            tipY = spear.rect.y + spear.rect.h;     // Bottom tip
            break;
        case Direction::DOWN: // Moving UP
            tipX = spear.rect.x + spear.rect.w / 2; // Mid-point of base
            tipY = spear.rect.y;                    // Top tip
            break;
        case Direction::LEFT: // Moving RIGHT
            tipX = spear.rect.x + spear.rect.w;     // Right tip
            tipY = spear.rect.y + spear.rect.h / 2; // Mid-point of base
            break;
        case Direction::RIGHT: // Moving LEFT
            tipX = spear.rect.x;                    // Left tip
            tipY = spear.rect.y + spear.rect.h / 2; // Mid-point of base
            break;
        case Direction::NONE: return false;
    }

    // Simple point-in-rect check for the tip
    return (tipX >= blockZone.x && tipX < blockZone.x + blockZone.w &&
            tipY >= blockZone.y && tipY < blockZone.y + blockZone.h);
}

// Spawn a new spear from a random edge, targeting the center
void SpawnSpear(std::vector<Spear>& spears, const GameSettings& settings) {
    Spear newSpear;
    newSpear.speed = settings.spearSpeed;
    int side = rand() % 4; // 0: top, 1: bottom, 2: left, 3: right

    // Define dimensions based on orientation
    int width, height;
    if (side == 0 || side == 1) { // Vertical (from Top/Bottom)
        width = SPEAR_BASE_WIDTH;
        height = SPEAR_LENGTH;
    } else { // Horizontal (from Left/Right)
        width = SPEAR_LENGTH;
        height = SPEAR_BASE_WIDTH;
    }
    newSpear.rect.w = width;
    newSpear.rect.h = height;


    switch (side) {
        case 0: // Top
            newSpear.originDirection = Direction::UP;
            newSpear.x = static_cast<float>((SCREEN_WIDTH - width) / 2);
            newSpear.y = static_cast<float>(-height); // Start just above screen
            break;
        case 1: // Bottom
            newSpear.originDirection = Direction::DOWN;
            newSpear.x = static_cast<float>((SCREEN_WIDTH - width) / 2);
            newSpear.y = static_cast<float>(SCREEN_HEIGHT); // Start just below screen
            break;
        case 2: // Left
            newSpear.originDirection = Direction::LEFT;
            newSpear.x = static_cast<float>(-width); // Start just left of screen
            newSpear.y = static_cast<float>((SCREEN_HEIGHT - height) / 2);
            break;
        case 3: // Right
            newSpear.originDirection = Direction::RIGHT;
            newSpear.x = static_cast<float>(SCREEN_WIDTH); // Start just right of screen
            newSpear.y = static_cast<float>((SCREEN_HEIGHT - height) / 2);
            break;
    }
    // Initialize rect position from float
    newSpear.rect.x = static_cast<int>(newSpear.x);
    newSpear.rect.y = static_cast<int>(newSpear.y);

    spears.push_back(newSpear);
}

// Render based on the current game state
void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Player& player, const std::vector<Spear>& spears, GameState gameState, int selectedOption, bool gameOverFlag) {
    // Clear screen (black background)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (gameState == GameState::MENU) {
        RenderMenu(renderer, font, selectedOption);
    } else if (gameState == GameState::PLAYING || gameState == GameState::GAME_OVER) {
        // --- Render Gameplay Elements ---

        // Draw Player (Red square)
        SDL_Color playerColor = {255, 0, 0, 255}; // Normal red
        if (gameState == GameState::GAME_OVER) {
             playerColor = {100, 0, 0, 255}; // Darker red on game over
        }
        SDL_SetRenderDrawColor(renderer, playerColor.r, playerColor.g, playerColor.b, playerColor.a);
        SDL_RenderFillRect(renderer, &player.rect);

        // Draw Facing Indicator (Small yellow square) - Only if not game over
        if (gameState == GameState::PLAYING) {
            SDL_Rect indicatorRect;
            indicatorRect.w = INDICATOR_SIZE;
            indicatorRect.h = INDICATOR_SIZE;
            int playerCenterX = player.rect.x + player.rect.w / 2;
            int playerCenterY = player.rect.y + player.rect.h / 2;

            switch (player.facing) {
                case Direction::UP:    indicatorRect.x = playerCenterX - INDICATOR_SIZE / 2; indicatorRect.y = player.rect.y - INDICATOR_SIZE - 2; break;
                case Direction::DOWN:  indicatorRect.x = playerCenterX - INDICATOR_SIZE / 2; indicatorRect.y = player.rect.y + player.rect.h + 2; break;
                case Direction::LEFT:  indicatorRect.x = player.rect.x - INDICATOR_SIZE - 2; indicatorRect.y = playerCenterY - INDICATOR_SIZE / 2; break;
                case Direction::RIGHT: indicatorRect.x = player.rect.x + player.rect.w + 2; indicatorRect.y = playerCenterY - INDICATOR_SIZE / 2; break;
                case Direction::NONE: break;
            }
            if (player.facing != Direction::NONE) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow indicator
                SDL_RenderFillRect(renderer, &indicatorRect);
            }
        }

        // Draw Spears (Triangles)
        SDL_SetRenderDrawColor(renderer, 0, 180, 255, 255); // Lighter blue for spears
        for (const auto& spear : spears) {
            RenderSpear(renderer, spear);
        }

        // --- Render Game Over Text (if applicable) ---
        if (gameState == GameState::GAME_OVER) {
            RenderGameOver(renderer, font);
        }
    }

    // Update screen
    SDL_RenderPresent(renderer);
}

// Render the main menu
void RenderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255}; // Highlight color

    // Title
    RenderText(renderer, font, "Spear Block Challenge", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4, white);

    // Difficulty Options
    RenderText(renderer, font, "Easy",   SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 0, (selectedOption == 0) ? yellow : white);
    RenderText(renderer, font, "Medium", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 40, (selectedOption == 1) ? yellow : white);
    RenderText(renderer, font, "Hard",   SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 80, (selectedOption == 2) ? yellow : white);

    // Instructions
     RenderText(renderer, font, "Use Arrow Keys/WASD to select, Enter/Space to start", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, white);
}

// Render the game over screen text
void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font) {
     SDL_Color red = {255, 50, 50, 255};
     SDL_Color white = {255, 255, 255, 255};
     RenderText(renderer, font, "GAME OVER", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, red);
     RenderText(renderer, font, "Press any key to return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20, white);
}

// Helper function to render text centered at x, y
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    if (!font) return; // Don't render if font is null

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color); // Use Blended for better quality
    if (!surface) {
        printf("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Unable to create texture from rendered text! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect destRect;
    destRect.w = surface->w;
    destRect.h = surface->h;
    destRect.x = x - destRect.w / 2; // Center horizontally
    destRect.y = y - destRect.h / 2; // Center vertically

    SDL_RenderCopy(renderer, texture, NULL, &destRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Render a single spear as a filled triangle
void RenderSpear(SDL_Renderer* renderer, const Spear& spear) {
    // Get spear dimensions and position
    int x = spear.rect.x;
    int y = spear.rect.y;
    int w = spear.rect.w; // Base width or length depending on orientation
    int h = spear.rect.h; // Length or base width depending on orientation

    // Define triangle vertices based on origin direction
    SDL_Vertex vertex[3];
    // Set color for all vertices
    vertex[0].color = vertex[1].color = vertex[2].color = {0, 180, 255, 255}; // Spear color

    switch (spear.originDirection) {
        case Direction::UP: // Moving DOWN, tip points down
            vertex[0].position = {(float)x, (float)y};             // Top-left base
            vertex[1].position = {(float)(x + w), (float)y};         // Top-right base
            vertex[2].position = {(float)(x + w / 2), (float)(y + h)}; // Bottom tip
            break;
        case Direction::DOWN: // Moving UP, tip points up
            vertex[0].position = {(float)x, (float)(y + h)};         // Bottom-left base
            vertex[1].position = {(float)(x + w), (float)(y + h)};     // Bottom-right base
            vertex[2].position = {(float)(x + w / 2), (float)y};         // Top tip
            break;
        case Direction::LEFT: // Moving RIGHT, tip points right
            vertex[0].position = {(float)x, (float)y};             // Top-left base
            vertex[1].position = {(float)x, (float)(y + h)};         // Bottom-left base
            vertex[2].position = {(float)(x + w), (float)(y + h / 2)}; // Right tip
            break;
        case Direction::RIGHT: // Moving LEFT, tip points left
            vertex[0].position = {(float)(x + w), (float)y};         // Top-right base
            vertex[1].position = {(float)(x + w), (float)(y + h)};     // Bottom-right base
            vertex[2].position = {(float)x, (float)(y + h / 2)};     // Left tip
            break;
        case Direction::NONE: return; // Don't draw if direction is none
    }

    // Render the filled triangle
    // Note: SDL_RenderGeometry requires SDL 2.0.10 or later.
    // If using an older version, you might need a different approach (e.g., SDL_gfx library or drawing lines).
    SDL_RenderGeometry(renderer, nullptr, vertex, 3, nullptr, 0);
}
