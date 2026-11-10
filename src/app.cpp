#include "app.hpp"
#include "font.hpp"
#include "sftp.hpp"
#include "backup.hpp"
#include <cstdlib>
#include <algorithm>

namespace rb {
namespace {
constexpr int W = 640, H = 480;
const SDL_Color bg{18, 22, 30, 255};
const SDL_Color panel{31, 39, 51, 255};
const SDL_Color white{235, 240, 245, 255};
const SDL_Color muted{145, 158, 174, 255};
const SDL_Color accent{104, 196, 156, 255};
const SDL_Color selected{54, 73, 88, 255};
}

App::App() = default;
App::~App() {
    delete configStore_;
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool App::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) return false;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    window_ = SDL_CreateWindow("Retro Backup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               W, H, SDL_WINDOW_SHOWN);
    if (!window_) return false;
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) return false;
    SDL_ShowCursor(SDL_DISABLE);
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);
    char* basePath = SDL_GetBasePath();
    if (!basePath) return false;
    configStore_ = new ConfigStore(basePath);
    SDL_free(basePath);
    std::string configError;
    if (!configStore_->load(config_, configError)) {
        settingsMessage_ = configError;
        configStore_->save(config_, configError);
    }
    status_ = config_.host.empty() ? "NOT CONFIGURED" : "SFTP CONFIGURED";
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) SDL_GameControllerOpen(i);
        else SDL_JoystickOpen(i);
    }
    return true;
}

int App::run() {
    while (running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) handleEvent(event);
        render();
        SDL_Delay(16);
    }
    return 0;
}

void App::drawPanel(int x, int y, int w, int h, SDL_Color color) {
    SDL_Rect r{x,y,w,h};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &r);
}

int App::selectionLimit() const {
    if (screen_ == Screen::Home) return static_cast<int>(homeItems_.size()) - 1;
    if (screen_ == Screen::History || screen_ == Screen::Restore)
        return backups_.empty() ? 0 : std::min(6, static_cast<int>(backups_.size()) - 1);
    return 0;
}

void App::loadHistory() {
    std::vector<std::string> found;
    std::string result;
    if (listRemoteBackups(config_, *configStore_, found, result)) backups_ = found;
    else backups_.clear();
    historyMessage_ = result;
    restoreConfirm_ = false;
    selection_ = 0;
}

void App::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) { running_ = false; return; }
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (e.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP: selection_ = std::max(0, selection_ - 1); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                selection_ = std::min(selectionLimit(), selection_ + 1); break;
            case SDL_CONTROLLER_BUTTON_A: activate(); break;
            case SDL_CONTROLLER_BUTTON_B: goBack(); break;
            case SDL_CONTROLLER_BUTTON_START: running_ = false; break;
            default: break;
        }
        return;
    }
    if (e.type == SDL_JOYBUTTONDOWN) {
        // Fallback for devices without an SDL GameController mapping.
        switch (e.jbutton.button) {
            case 0: activate(); break;
            case 1: goBack(); break;
            default: break;
        }
        return;
    }
    if (e.type != SDL_KEYDOWN || e.key.repeat) return;
    switch (e.key.keysym.sym) {
        case SDLK_UP: case SDLK_w:
            selection_ = std::max(0, selection_ - 1); break;
        case SDLK_DOWN: case SDLK_s:
            selection_ = std::min(selectionLimit(), selection_ + 1); break;
        case SDLK_RETURN: case SDLK_SPACE: case SDLK_z:
            activate(); break;
        case SDLK_ESCAPE: case SDLK_x: case SDLK_BACKSPACE:
            goBack(); break;
        case SDLK_q:
            running_ = false; break;
        default: break;
    }
}

void App::activate() {
    if (screen_ == Screen::History) { loadHistory(); return; }
    if (screen_ == Screen::Restore) {
        if (backups_.empty() || selection_ >= static_cast<int>(backups_.size())) { loadHistory(); return; }
        if (!restoreConfirm_) { restoreConfirm_ = true; historyMessage_ = "A AGAIN: RESTORE " + backups_[selection_]; return; }
        const std::string selectedBackup = backups_[selection_];
        std::string result;
        const bool ok = restoreRemoteBackup(config_, *configStore_, selectedBackup, result);
        historyMessage_ = result;
        restoreConfirm_ = false;
        status_ = ok ? "RESTORE COMPLETE" : "RESTORE ERROR";
        loadHistory();
        historyMessage_ = result;
        return;
    }
    if (screen_ == Screen::Backup) {
        std::string result;
        const bool ok = createAndUploadBackup(config_, *configStore_, result);
        backupMessage_ = result;
        status_ = ok ? "BACKUP COMPLETE" : "BACKUP ERROR";
        return;
    }
    if (screen_ == Screen::Settings) {
        std::string result;
        const bool ok = testSftpConnection(config_, *configStore_, result);
        settingsMessage_ = result;
        status_ = ok ? "SFTP ONLINE" : "SFTP ERROR";
        return;
    }
    if (screen_ != Screen::Home) { goBack(); return; }
    switch (selection_) {
        case 0: screen_ = Screen::Backup; break;
        case 1: screen_ = Screen::History; loadHistory(); break;
        case 2: screen_ = Screen::Restore; loadHistory(); break;
        case 3: screen_ = Screen::Settings; break;
        case 4: screen_ = Screen::About; break;
        default: running_ = false; break;
    }
    selection_ = 0;
}

void App::goBack() {
    if (screen_ != Screen::Home) { screen_ = Screen::Home; selection_ = 0; }
    else running_ = false;
}

void App::renderHome() {
    drawPanel(32, 84, 576, 340, panel);
    rb::drawText(renderer_, 64, 110, "RETRO BACKUP", accent, 4);
    rb::drawText(renderer_, 64, 160, "SERVER", muted, 2);
    rb::drawText(renderer_, 220, 160, status_, white, 2);
    rb::drawText(renderer_, 64, 190, "LAST BACKUP", muted, 2);
    rb::drawText(renderer_, 220, 190, "NOT AVAILABLE", white, 2);

    int y = 238;
    for (size_t i = 0; i < homeItems_.size(); ++i) {
        if (static_cast<int>(i) == selection_) drawPanel(56, y - 8, 528, 34, selected);
        rb::drawText(renderer_, 78, y, static_cast<int>(i) == selection_ ? "> " + homeItems_[i] : homeItems_[i],
                     static_cast<int>(i) == selection_ ? accent : white, 2);
        y += 38;
    }
}

void App::renderSettings() {
    drawPanel(32, 84, 576, 340, panel);
    rb::drawText(renderer_, 64, 112, "SFTP SETTINGS", accent, 3);
    const std::vector<std::string> lines = {
        "HOST: " + (config_.host.empty() ? "NOT SET" : config_.host),
        "PORT: " + config_.port,
        "USER: " + (config_.username.empty() ? "NOT SET" : config_.username),
        "REMOTE: " + config_.remotePath,
        "SSH KEY: " + config_.identityFile,
        "HOST KEYS: " + config_.knownHostsFile
    };
    int y = 165;
    for (const auto& line : lines) {
        rb::drawText(renderer_, 64, y, line, white, 1);
        y += 25;
    }
    rb::drawText(renderer_, 64, 335, settingsMessage_, accent, 1);
    rb::drawText(renderer_, 64, 380, "A: TEST CONNECTION    B: BACK", muted, 2);
}

void App::renderInfo(const std::string& title, const std::vector<std::string>& lines) {
    drawPanel(32, 84, 576, 340, panel);
    rb::drawText(renderer_, 64, 112, title, accent, 3);
    int y = 180;
    for (const auto& line : lines) {
        rb::drawText(renderer_, 64, y, line, white, 2);
        y += 34;
    }
    rb::drawText(renderer_, 64, 380, "B: BACK", muted, 2);
}

void App::renderBackupList(bool restoreMode) {
    drawPanel(32, 84, 576, 340, panel);
    rb::drawText(renderer_, 64, 112, restoreMode ? "RESTORE BACKUP" : "BACKUP HISTORY", accent, 3);
    rb::drawText(renderer_, 64, 154, historyMessage_, muted, 1);
    if (backups_.empty()) {
        rb::drawText(renderer_, 64, 210, "NO COMPLETED BACKUPS", white, 2);
    } else {
        int y = 190;
        const int count = std::min(7, static_cast<int>(backups_.size()));
        for (int i = 0; i < count; ++i) {
            if (i == selection_) drawPanel(54, y - 5, 532, 30, selected);
            rb::drawText(renderer_, 70, y, (i == selection_ ? "> " : "  ") + backups_[i],
                         i == selection_ ? accent : white, 1);
            y += 29;
        }
    }
    if (restoreMode && restoreConfirm_)
        rb::drawText(renderer_, 64, 382, "PRESS A AGAIN TO CONFIRM RESTORE", accent, 1);
    else
        rb::drawText(renderer_, 64, 382, restoreMode ? "A: SELECT / CONFIRM RESTORE" : "A: REFRESH", muted, 1);
    rb::drawText(renderer_, 64, 402, "B: BACK", muted, 1);
}

void App::render() {
    SDL_SetRenderDrawColor(renderer_, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(renderer_);
    rb::drawText(renderer_, 32, 28, "RETRO BACKUP", white, 2);
    rb::drawText(renderer_, 490, 28, "MVP 0.3", muted, 2);

    switch (screen_) {
        case Screen::Home: renderHome(); break;
        case Screen::Backup:
            renderInfo("BACKUP NOW", {backupMessage_, "A: START / RETRY BACKUP", "B: BACK"});
            break;
        case Screen::History: renderBackupList(false); break;
        case Screen::Restore: renderBackupList(true); break;
        case Screen::Settings: renderSettings(); break;
        case Screen::About:
            renderInfo("ABOUT", {"RETRO BACKUP 0.2.0", "ONIONOS COMMUNITY APP", "OPEN SOURCE PROJECT"});
            break;
    }
    rb::drawText(renderer_, 32, 448, "D-PAD: MOVE    A: SELECT    B: BACK", muted, 1);
    SDL_RenderPresent(renderer_);
}
}
