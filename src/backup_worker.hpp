#pragma once
#include "backup.hpp"
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace rb {
class BackupWorker {
public:
    BackupWorker(Config config, const ConfigStore& store);
    ~BackupWorker();
    bool request();
    bool busy() const;
    std::string message() const;
private:
    void loop();
    Config config_;
    const ConfigStore& store_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_{false};
    bool requested_{true}; // One automatic backup immediately after app startup.
    bool busy_{false};
    std::string message_{"BACKUP QUEUED"};
    std::thread thread_;
};
}
