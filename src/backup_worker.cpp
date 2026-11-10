#include "backup_worker.hpp"
#include <utility>

namespace rb {
BackupWorker::BackupWorker(Config config, const ConfigStore& store)
    : config_(std::move(config)), store_(store), thread_(&BackupWorker::loop, this) {}

BackupWorker::~BackupWorker() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

bool BackupWorker::request() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopping_ || busy_ || requested_) return false;
    requested_ = true;
    message_ = "BACKUP QUEUED";
    cv_.notify_one();
    return true;
}

bool BackupWorker::busy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return busy_ || requested_;
}

std::string BackupWorker::message() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return message_;
}

void BackupWorker::loop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stopping_) {
        cv_.wait(lock, [this] { return stopping_ || requested_; });
        if (stopping_) break;
        requested_ = false;
        busy_ = true;
        message_ = "BACKUP RUNNING";
        lock.unlock();

        std::string result;
        bool ok = false;
        try { ok = createAndUploadBackup(config_, store_, result); }
        catch (...) { result = "BACKUP FAILED - INTERNAL ERROR"; }

        lock.lock();
        busy_ = false;
        message_ = result.empty() ? (ok ? "BACKUP COMPLETE" : "BACKUP FAILED") : result;
    }
}
}
