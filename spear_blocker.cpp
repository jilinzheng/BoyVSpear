#include "spear_blocker.h"
#include <ctime>   // For time()
#include <iostream> // For cout
#include <fstream>  // For ifstream (reading files/FIFOs)
#include <string>   // For string manipulation
#include <unistd.h> // For sleep (optional)
#include <sys/stat.h> // For checking file type (optional but good)
#include <sys/types.h>
#include <fcntl.h> // For low-level open (alternative)
#include <cerrno>   // For errno
#include <cstdio>  // For perror


int SPEAR_COUNTER = 0; // Counter for spears
int RETURN_TO_MENU = 0; // Flag to return to menu
const int BLOCK_ZONE_SIZE = PLAYER_SIZE + 20; // Keep block zone relative
const char* FIFO_PATH = "/tmp/joystick_fifo";


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

    // FIFO to read BLE values written by Python BLE client
    std::ifstream fifo_stream;
    std::string line;
    int joy_x_cmd, joy_y_cmd, joy_btn_press;

    // --- Game Loop ---
    while (running) {
        // Check if stream is open, if not, try to open it.
        if (!fifo_stream.is_open()) {
            // Optional: Check if the FIFO file exists and is a FIFO before opening
            struct stat stat_buf;
            if (stat(FIFO_PATH, &stat_buf) == 0) {
                if (!S_ISFIFO(stat_buf.st_mode)) {
                    std::cerr << "Error: " << FIFO_PATH << " exists but is not a FIFO." << std::endl;
                    sleep(5); // Wait before retrying
                    continue;
                }
            } else {
                // File doesn't exist yet, wait for Python script to create it
                if (errno == ENOENT) {
                std::cout << "FIFO not found, waiting..." << std::endl;
                } else {
                perror("Error checking FIFO status"); // Other stat error
                }
                sleep(2);
                continue;
            }

            // Open the FIFO for reading [4] [6]
            // This will block until the Python script opens it for writing [6]
            std::cout << "Attempting to open FIFO: " << FIFO_PATH << std::endl;
            fifo_stream.open(FIFO_PATH); // Opens in read mode by default

            if (!fifo_stream.is_open()) {
                std::cerr << "Error opening FIFO: " << FIFO_PATH << ". Retrying..." << std::endl;
                // perror("open"); // Can use perror if using low-level open()
                sleep(2); // Wait before retrying
                continue;
            } else {
                std::cout << "FIFO opened successfully." << std::endl;
            }
        }

        // Read line by line from the FIFO stream
        if (std::getline(fifo_stream, line)) {
            // Process the received line (X Y Button)
            std::cout << "Received: " << line << std::endl;

            // Example of parsing (optional):
            std::stringstream ss(line);
            // int x, y, btn;
            if (ss >> joy_x_cmd> joy_y_cmd >> joy_btn_press) {
               std::cout << "Parsed -> X: " << joy_x_cmd << ", Y: " << joy_y_cmd << ", Btn: " << joy_btn_press << std::endl;
            } else {
               std::cerr << "Warning: Could not parse line: " << line << std::endl;
            }

        } else {
            // getline failed. This could mean the writer closed the pipe (EOF)
            // or some other error occurred.
            if (fifo_stream.eof()) {
                std::cout << "Writer closed the FIFO (EOF reached). Re-opening..." << std::endl;
            } else if (fifo_stream.fail()) {
                std::cerr << "Stream error occurred. Re-opening..." << std::endl;
            } else {
                 std::cerr << "Unknown stream state. Re-opening..." << std::endl;
            }
            fifo_stream.close();    // Close the stream
            fifo_stream.clear();    // Clear error flags
            sleep(1); // Small delay before trying to reopen
            continue;
        }
        startGame = false;

        if(HandleInput(running, player, gameState, menuSelectedOption, difficulty, startGame,
            joy_x_cmd, joy_y_cmd, joy_btn_press) == -1)
        {
            return -1;
        }

        if(!running) break;

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

int HandleInput(bool& running, Player& player, GameState& gameState, int& selectedOption, Difficulty& difficulty, bool& startGame,
    int joy_x_cmd, int joy_y_cmd, int joy_btn_press){
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) { running = false; return -1;}

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
                    case SDLK_z: running = false; break;
                }
            }
            // joystick
            if (joy_y_cmd == UP) selectedOption = (selectedOption-1+4)%4;
            if (joy_y_cmd == DOWN) selectedOption = (selectedOption+1)%4;
            if (joy_btn_press == PRESSED) {
                if (selectedOption == 0) difficulty = Difficulty::EASY;
                else if (selectedOption == 1) difficulty = Difficulty::MEDIUM;
                else if (selectedOption==2) difficulty = Difficulty::HARD;
                else RETURN_TO_MENU = true;
                startGame = true;
            }

        } else if (gameState == GameState::PLAYING) {
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:    case SDLK_w: player.facing = Direction::UP; break;
                    case SDLK_DOWN:  case SDLK_s: player.facing = Direction::DOWN; break;
                    case SDLK_LEFT:  case SDLK_a: player.facing = Direction::LEFT; break;
                    case SDLK_RIGHT: case SDLK_d: player.facing = Direction::RIGHT; break;
                }
            }
            // joystick
            if (joy_y_cmd == UP) player.facing = Direction::UP;
            if (joy_y_cmd == DOWN) player.facing = Direction::DOWN;
            if (joy_x_cmd == LEFT) player.facing = Direction::LEFT;
            if (joy_x_cmd == RIGHT) player.facing = Direction::RIGHT;

        } else if (gameState == GameState::GAME_OVER) {
            if (e.type == SDL_KEYDOWN) {
                gameState = GameState::MENU;
                selectedOption = 0;
            }
            // joystick
            if (joy_x_cmd!=NEUTRAL||joy_y_cmd!=NEUTRAL||joy_btn_press==PRESSED) {
                gameState = GameState::MENU;
                selectedOption = 0;
            }
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
