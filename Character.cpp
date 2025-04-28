#include "Character.h"
// Character body height/width reference

// Helper function to draw a circle outline using points

void DrawCircle(SDL_Renderer* renderer, int centreX, int centreY, int radius) {
    const int diameter = (radius * 2);
    int x = (radius - 1); int y = 0; int tx = 1; int ty = 1;
    int error = (tx - diameter);
    while (x >= y) {
        SDL_RenderDrawPoint(renderer, centreX + x, centreY - y); SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
        SDL_RenderDrawPoint(renderer, centreX - x, centreY - y); SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
        SDL_RenderDrawPoint(renderer, centreX + y, centreY - x); SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
        SDL_RenderDrawPoint(renderer, centreX - y, centreY - x); SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);
        if (error <= 0) { ++y; error += ty; ty += 2; }
        if (error > 0) { --x; tx += 2; error += (tx - diameter); }
    }
}

// Helper function to draw a filled circle by drawing horizontal lines
void FillCircle(SDL_Renderer* renderer, int centreX, int centreY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centreX + dx, centreY + dy);
            }
        }
    }
}

// Render the player character, shield position indicates facing direction
void RenderPlayerCharacter(SDL_Renderer* renderer, const Player& player, bool isGameOver, int Game_Type) {
    // Calculate base character dimensions and positions
    int centerX = static_cast<int>(player.x);
    int centerY = static_cast<int>(player.y);
    int bodyHeight = PLAYER_SIZE;
    int bodyWidth = static_cast<int>(PLAYER_SIZE * 0.6f);
    int headRadius = static_cast<int>(bodyHeight * 0.25f);
    int headY = centerY - static_cast<int>(bodyHeight * 0.25f);

    SDL_Rect bodyRect;
    bodyRect.w = bodyWidth;
    bodyRect.h = static_cast<int>(bodyHeight * 0.6f);
    bodyRect.x = centerX - bodyWidth / 2;
    bodyRect.y = headY + headRadius - 5;

    // Determine body color based on game over state
    SDL_Color bodyColor = {0, 128, 0, 255}; // Normal green
    if (isGameOver) {
        bodyColor = {0, 60, 0, 255}; // Darker green
    }

    // --- Draw Base Character ---
    // Head
    SDL_SetRenderDrawColor(renderer, 255, 224, 189, 255); // Skin color
    FillCircle(renderer, centerX, headY, headRadius); // Fill head
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black outline for head
    DrawCircle(renderer, centerX, headY, headRadius); // Outline head
    // Body
    SDL_SetRenderDrawColor(renderer, bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a);
    SDL_RenderFillRect(renderer, &bodyRect);
    // Eyes
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
    int eyeOffsetX = headRadius / 2;
    int eyeOffsetY = headRadius / 4;
    // Draw slightly larger eyes
    FillCircle(renderer, centerX - eyeOffsetX, headY - eyeOffsetY, 2);
    FillCircle(renderer, centerX + eyeOffsetX, headY - eyeOffsetY, 2);
    // SDL_RenderDrawPoint(renderer, centerX - eyeOffsetX, headY - eyeOffsetY); // Original single point eyes
    // SDL_RenderDrawPoint(renderer, centerX + eyeOffsetX, headY - eyeOffsetY);
    // Mouth
    int mouthY = headY + eyeOffsetY;
    SDL_RenderDrawLine(renderer, centerX - eyeOffsetX, mouthY, centerX + eyeOffsetX, mouthY);

    // --- Draw Shield based on player.facing (only if not game over) ---
    if (!isGameOver) {
        int shieldRadius = headRadius + 2; // Slightly larger shield
        int shieldX = centerX;
        int shieldY = centerY;
        // int shieldOffset = bodyWidth / 2 + shieldRadius + 5; // Original offset calculation (unused)

        switch (player.facing) {
            case Direction::UP:
                shieldY = bodyRect.y - shieldRadius - 3;
                shieldX = centerX;
                break;
            case Direction::DOWN:
                shieldY = bodyRect.y + bodyRect.h + shieldRadius + 3;
                shieldX = centerX;
                break;
            case Direction::LEFT:
                shieldX = bodyRect.x - shieldRadius - 3;
                shieldY = bodyRect.y + bodyRect.h / 2;
                break;
            case Direction::RIGHT:
                shieldX = bodyRect.x + bodyRect.w + shieldRadius + 3;
                shieldY = bodyRect.y + bodyRect.h / 2;
                break;
            case Direction::NONE:
                return; // Don't draw shield if direction is none
        }
        if (Game_Type == 1)
        {
            // Draw the shield: Fill first, then outline
            SDL_SetRenderDrawColor(renderer, 169, 169, 169, 255); // Shield silver
            FillCircle(renderer, shieldX, shieldY, shieldRadius); // Use FillCircle

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Shield outline (Black)
            DrawCircle(renderer, shieldX, shieldY, shieldRadius); // Use DrawCircle for outline
        }
    }
}

