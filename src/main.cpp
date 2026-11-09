#include "app.hpp"
#include <cstdio>

int main() {
    rb::App app;
    if (!app.init()) {
        std::fprintf(stderr, "Retro Backup: SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    return app.run();
}
