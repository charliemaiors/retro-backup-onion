#pragma once
#include <SDL.h>
#include <string>

namespace rb {
void drawText(SDL_Renderer* renderer, int x, int y, const std::string& text,
              SDL_Color color, int scale = 2);
}
