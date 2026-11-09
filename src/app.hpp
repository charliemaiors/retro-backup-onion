#pragma once
#include <SDL.h>
#include <string>
#include <vector>

namespace rb {
class App {
public:
    App();
    ~App();
    bool init();
    int run();

private:
    enum class Screen { Home, Backup, History, Restore, Settings, About };
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    bool running_{true};
    Screen screen_{Screen::Home};
    int selection_{0};
    std::string status_{"NOT CONFIGURED"};
    std::vector<std::string> homeItems_{
        "BACKUP NOW", "BACKUP HISTORY", "RESTORE", "SETTINGS", "ABOUT", "EXIT"
    };
    void handleEvent(const SDL_Event& event);
    void render();
    void renderHome();
    void renderInfo(const std::string& title, const std::vector<std::string>& lines);
    void activate();
    void goBack();
    void drawPanel(int x, int y, int w, int h, SDL_Color color);
};
}
