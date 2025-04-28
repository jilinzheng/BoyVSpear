#include "menu.h"

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

void RenderGameOver(SDL_Renderer* renderer, TTF_Font* font, int score) {
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color white = {255, 255, 255, 255};
    RenderText(renderer, font, "GAME OVER", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, red);
    RenderText(renderer, font, "Your Score : " + std::to_string(score), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20, white);
}

void RenderScore(SDL_Renderer* renderer, TTF_Font* font, int score) {
    if (!renderer || !font) return; // Safety check

    SDL_Color white = {255, 255, 255, 255};
    std::string scoreText = "Score: " + std::to_string(score);

    SDL_Surface* scoreSurface = TTF_RenderText_Blended(font, scoreText.c_str(), white);
    if (scoreSurface) {
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        if (scoreTexture) {
            SDL_Rect scoreRect;
            scoreRect.x = 10; // Top-left x
            scoreRect.y = 10; // Top-left y
            scoreRect.w = scoreSurface->w;
            scoreRect.h = scoreSurface->h;
            SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);
            SDL_DestroyTexture(scoreTexture);
        }
        SDL_FreeSurface(scoreSurface);
    }
}

