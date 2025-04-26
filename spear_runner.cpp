#include <SDL2/SDL.h>
#include <vector>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

// --- Configuration ---
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int PLAYER_SIZE = 20;
const int PLAYER_SPEED = 5;
const int SPEAR_WIDTH = 5;
const int SPEAR_HEIGHT = 30; // Vertical spears
const int SPEAR_LENGTH = 30; // Horizontal spears (use width for horizontal)
const int SPEAR_SPEED = 4;
const int SPEAR_SPAWN_RATE = 30; // Lower number = more frequent spawns (every X frames)

// --- Structs ---
struct Player {
    SDL_Rect rect; // Position and size
    int dx = 0;    // Horizontal velocity
    int dy = 0;    // Vertical velocity
};

enum class SpearDirection {
    UP, DOWN, LEFT, RIGHT
};

struct Spear {
    SDL_Rect rect;
    SpearDirection direction;
};

// --- Function Prototypes ---
bool InitializeSDL(SDL_Window*& window, SDL_Renderer*& renderer);
void CloseSDL(SDL_Window* window, SDL_Renderer* renderer);
void HandleInput(bool& running, Player& player);
void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver);
void SpawnSpear(std::vector<Spear>& spears);
void RenderGame(SDL_Renderer* renderer, const Player& player, const std::vector<Spear>& spears);
bool CheckCollision(const SDL_Rect& a, const SDL_Rect& b);

// --- Main Function ---
int main(int argc, char* args[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!InitializeSDL(window, renderer)) {
        printf("Failed to initialize SDL!\n");
        return 1;
    }

    // Seed random number generator
    srand(time(0));

    // Game variables
    bool running = true;
    bool gameOver = false;
    SDL_Event e;
    int frameCount = 0;

    // Initialize Player
    Player player;
    player.rect.w = PLAYER_SIZE;
    player.rect.h = PLAYER_SIZE;
    player.rect.x = (SCREEN_WIDTH - PLAYER_SIZE) / 2;
    player.rect.y = (SCREEN_HEIGHT - PLAYER_SIZE) / 2;

    // Initialize Spears vector
    std::vector<Spear> spears;

    // --- Game Loop ---
    while (running) {
        // --- Input Handling ---
        HandleInput(running, player);

        if (!gameOver) {
            // --- Update ---
            UpdateGame(player, spears, gameOver);

            // Spawn spears periodically
            frameCount++;
            if (frameCount >= SPEAR_SPAWN_RATE) {
                SpawnSpear(spears);
                frameCount = 0; // Reset counter
            }

            // --- Collision Check ---
            for (const auto& spear : spears) {
                if (CheckCollision(player.rect, spear.rect)) {
                    gameOver = true;
                    printf("Game Over!\n");
                    // In a real game, you'd show a game over screen here
                    // For simplicity, we'll just stop updates and wait to close
                    break; // Exit loop once collision detected
                }
            }
        } else {
             // If game is over, maybe wait for a key press to exit or restart
             // For now, the game loop continues but nothing updates
             // The user can close the window via HandleInput
        }


        // --- Rendering ---
        RenderGame(renderer, player, spears);

        // Add a small delay to control frame rate
        SDL_Delay(16); // Roughly 60 FPS
    }

    // --- Cleanup ---
    CloseSDL(window, renderer);

    return 0;
}

// --- Function Definitions ---

// Initialize SDL, create window and renderer
bool InitializeSDL(SDL_Window*& window, SDL_Renderer*& renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Spear Dodge (Undertale Inspired)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    return true;
}

// Clean up SDL resources
void CloseSDL(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = nullptr;
    window = nullptr;
    SDL_Quit();
}

// Handle user input events
void HandleInput(bool& running, Player& player) {
    SDL_Event e;
    // Reset velocity at the start of each frame's input check
    player.dx = 0;
    player.dy = 0;

    // Get current keyboard state
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

    if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W]) {
        player.dy = -PLAYER_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S]) {
        player.dy = PLAYER_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A]) {
        player.dx = -PLAYER_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D]) {
        player.dx = PLAYER_SPEED;
    }

     // Handle window close event
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            running = false;
        }
         // Optional: Handle key release if needed (not necessary for continuous movement)
        /* else if (e.type == SDL_KEYUP) {
             switch (e.key.keysym.sym) {
                 // Handle key releases if you want movement to stop instantly on release
             }
         }*/
    }
}


// Update player position and spear positions
void UpdateGame(Player& player, std::vector<Spear>& spears, bool& gameOver) {
    // Update player position
    player.rect.x += player.dx;
    player.rect.y += player.dy;

    // Keep player within screen bounds
    if (player.rect.x < 0) {
        player.rect.x = 0;
    } else if (player.rect.x + player.rect.w > SCREEN_WIDTH) {
        player.rect.x = SCREEN_WIDTH - player.rect.w;
    }
    if (player.rect.y < 0) {
        player.rect.y = 0;
    } else if (player.rect.y + player.rect.h > SCREEN_HEIGHT) {
        player.rect.y = SCREEN_HEIGHT - player.rect.h;
    }

    // Update spear positions and remove spears that go off-screen
    for (int i = spears.size() - 1; i >= 0; --i) {
        bool removeSpear = false;
        switch (spears[i].direction) {
            case SpearDirection::UP:
                spears[i].rect.y -= SPEAR_SPEED;
                if (spears[i].rect.y + spears[i].rect.h < 0) removeSpear = true;
                break;
            case SpearDirection::DOWN:
                spears[i].rect.y += SPEAR_SPEED;
                 if (spears[i].rect.y > SCREEN_HEIGHT) removeSpear = true;
                break;
            case SpearDirection::LEFT:
                spears[i].rect.x -= SPEAR_SPEED;
                 if (spears[i].rect.x + spears[i].rect.w < 0) removeSpear = true;
                break;
            case SpearDirection::RIGHT:
                spears[i].rect.x += SPEAR_SPEED;
                 if (spears[i].rect.x > SCREEN_WIDTH) removeSpear = true;
                break;
        }

        if (removeSpear) {
            spears.erase(spears.begin() + i);
        }
    }
}

// Spawn a new spear from a random direction
void SpawnSpear(std::vector<Spear>& spears) {
    Spear newSpear;
    int side = rand() % 4; // 0: top, 1: bottom, 2: left, 3: right

    switch (side) {
        case 0: // Top
            newSpear.direction = SpearDirection::DOWN;
            newSpear.rect.w = SPEAR_WIDTH;
            newSpear.rect.h = SPEAR_HEIGHT;
            newSpear.rect.x = rand() % (SCREEN_WIDTH - newSpear.rect.w);
            newSpear.rect.y = -newSpear.rect.h; // Start just above screen
            break;
        case 1: // Bottom
            newSpear.direction = SpearDirection::UP;
            newSpear.rect.w = SPEAR_WIDTH;
            newSpear.rect.h = SPEAR_HEIGHT;
            newSpear.rect.x = rand() % (SCREEN_WIDTH - newSpear.rect.w);
            newSpear.rect.y = SCREEN_HEIGHT; // Start just below screen
            break;
        case 2: // Left
            newSpear.direction = SpearDirection::RIGHT;
            newSpear.rect.w = SPEAR_LENGTH; // Use length for horizontal
            newSpear.rect.h = SPEAR_WIDTH;  // Use width for horizontal
            newSpear.rect.x = -newSpear.rect.w; // Start just left of screen
            newSpear.rect.y = rand() % (SCREEN_HEIGHT - newSpear.rect.h);
            break;
        case 3: // Right
            newSpear.direction = SpearDirection::LEFT;
            newSpear.rect.w = SPEAR_LENGTH; // Use length for horizontal
            newSpear.rect.h = SPEAR_WIDTH;  // Use width for horizontal
            newSpear.rect.x = SCREEN_WIDTH; // Start just right of screen
            newSpear.rect.y = rand() % (SCREEN_HEIGHT - newSpear.rect.h);
            break;
    }
    spears.push_back(newSpear);
}

// Render all game objects
void RenderGame(SDL_Renderer* renderer, const Player& player, const std::vector<Spear>& spears) {
    // Clear screen (black background)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw Player (Red)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &player.rect);

    // Draw Spears (Blue)
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    for (const auto& spear : spears) {
        SDL_RenderFillRect(renderer, &spear.rect);
    }

    // Update screen
    SDL_RenderPresent(renderer);
}

// Simple Axis-Aligned Bounding Box (AABB) collision detection
bool CheckCollision(const SDL_Rect& a, const SDL_Rect& b) {
    // The sides of the rectangles
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    // Calculate the sides of rect A
    leftA = a.x;
    rightA = a.x + a.w;
    topA = a.y;
    bottomA = a.y + a.h;

    // Calculate the sides of rect B
    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;

    // If any of the sides from A are outside of B
    if (bottomA <= topB) { return false; }
    if (topA >= bottomB) { return false; }
    if (rightA <= leftB) { return false; }
    if (leftA >= rightB) { return false; }

    // If none of the sides from A are outside B
    return true;
}
