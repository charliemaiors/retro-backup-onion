#pragma once
#include <SDL.h>
#include "config.hpp"
#include <string>
#include <vector>
#include <memory>
#include "backup_worker.hpp"

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
    std::string settingsMessage_{"A: TEST SFTP CONNECTION"};
    std::string backupMessage_{"A: START MANUAL BACKUP"};
    std::string historyMessage_{"A: REFRESH HISTORY"};
    std::vector<std::string> backups_;
    bool restoreConfirm_{false};
    Config config_;
    std::unique_ptr<BackupWorker> backupWorker_;
    ConfigStore* configStore_{nullptr};
    std::vector<std::string> homeItems_{
        "BACKUP NOW", "BACKUP HISTORY", "RESTORE", "SETTINGS", "ABOUT", "EXIT"
    };
    void handleEvent(const SDL_Event& event);
    void render();
    void renderHome();
    void renderSettings();
    void renderInfo(const std::string& title, const std::vector<std::string>& lines);
    void activate();
    void goBack();
    void drawPanel(int x, int y, int w, int h, SDL_Color color);
    int selectionLimit() const;
    void loadHistory();
    void renderBackupList(bool restoreMode);
};
}
